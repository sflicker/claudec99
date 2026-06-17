# stage-134 Transcript: Bit-Field and Flexible Array Members in Structs

## Summary

Stage 134 addresses two C99 struct-member parsing bugs (CC99-006 and CC99-007). Bit-field members in structs were being rejected at the `:` token; the fix restructures the struct declarator parser to detect `:` after a declarator (named bit-field) or before it (anonymous bit-field), evaluate the width via constant-expression evaluation, and pack consecutive bit-fields into storage units using GCC-compatible layout rules. Flexible array members (unsized arrays as the last member) were rejected unconditionally; the fix allows them when at least one prior named member exists, creates a length-0 array type, and excludes the FAM from sizeof calculations.

The codegen implementation adds bit-field rvalue reads (shift-right and mask after loading the storage unit) and lvalue writes (read-modify-write with clear, shift, OR, store). Flexible arrays leverage the existing array-decay path to return pointers, requiring no codegen changes.

## Changes Made

### Step 1: Type System

- Added five new fields to `StructField` in `include/type.h`: `is_bitfield` (flag), `bit_width` (width in bits), `bit_offset` (offset within storage unit in bits), `is_flexible_array` (flag), and adjusted layout as needed to preserve struct size.
- Updated `create_struct_field` or equivalent allocation sites to initialize new fields to 0/false.

### Step 2: Parser (Bit-Field Support)

- Restructured `parse_struct_specifier` in `src/parser.c` to handle bit-field declarators alongside regular declarators.
- After parsing a declarator (or detecting `:` before any declarator for anonymous bit-fields), check for `:` token.
- If `:` is found, parse the constant-expression width using `eval_const_expr`, validate it as a positive integer, then record the bit-field metadata (width, offset, storage unit alignment).
- Implement GCC-compatible bit-field packing: consecutive bit-fields of the same base type that fit within the remaining bits of the current storage unit share that unit; otherwise, start a new unit at natural alignment. A zero-width bit-field closes the current unit and starts a new one.
- Added forward declaration of `eval_const_expr` if not already present.

### Step 3: Parser (Flexible Array Member Support)

- In `parse_struct_specifier`, after parsing a declarator and detecting it is an unsized array (in the `d.is_array && !d.has_size` branch), instead of unconditionally erroring, check that `tmp_fields_vec.len > 0` (at least one prior named member exists).
- If the check passes, create a `type_array(elem_type, 0)` field (length 0) without advancing `current_offset`, and set `is_flexible_array = 1` on the `StructField`.
- Validate "must be last" by checking on the next iteration if a previous field had `is_flexible_array` set; if so, reject the new field.
- If the struct has only the flexible member (no prior fields), reject with "struct with only flexible array member".

### Step 4: Codegen (Bit-Field Rvalue Reads)

- In `codegen_expression`, under the `AST_MEMBER_ACCESS` rvalue block (after `emit_member_addr` leaves the storage unit address in rax):
  - Check if the field is a bit-field (`f->is_bitfield`).
  - If true, load the storage unit with `mov eax, [rax]` (32-bit, or use the appropriate size for the storage unit type).
  - Shift right by `bit_offset`: `shr eax, <bit_offset>`.
  - Mask to `bit_width` bits: `and eax, (1 << <bit_width>) - 1`.
  - Result is in eax (as an rvalue).
- Apply the same logic to the `AST_ARROW_ACCESS` rvalue block.

### Step 5: Codegen (Bit-Field Lvalue Writes)

- In `codegen_expression`, under the compound-assignment or simple-assignment lvalue-write paths for `AST_MEMBER_ACCESS`:
  - After evaluating the RHS → eax, mask the new value to `bit_width` bits: `and eax, <mask>`.
  - Shift left by `bit_offset`: `shl eax, <bit_offset>` → ecx (save the shifted value).
  - Push the storage unit address (from the earlier member-address calculation).
  - Load the storage unit: `mov eax, [address]`.
  - Clear the field bits: `and eax, <clear_mask>` (where `clear_mask` has 0s in the bit-field range and 1s everywhere else).
  - OR in the new bits: `or eax, ecx`.
  - Store back: `mov [address], eax`.
- Apply the same logic to the `AST_ARROW_ACCESS` lvalue-write paths (both simple assignment and compound-assignment operators).

### Step 6: Tests

- Added `test/valid/structs/test_bit_field_members__23.c`: CC99-006 reduced example with three unsigned int bit-fields (low, mid, high with widths 3, 5, 10). Program assigns and reads back, returns 23.
- Added `test/valid/structs/test_bit_field_adjacent__5.c`: Two adjacent bit-fields of the same type sharing one storage unit. Returns sum of their values (5).
- Added `test/valid/structs/test_bit_field_anonymous__7.c`: Anonymous padding bit-field to align the next field. Returns computed value (7).
- Added `test/valid/structs/test_bit_field_zero_width__9.c`: Zero-width bit-field forcing a new storage unit. Returns value after unit separation (9).
- Added `test/valid/structs/test_flexible_array_member__34.c`: CC99-007 reduced example with malloc'd struct containing a FAM. Accesses FAM elements, returns computed value (34).
- Added `test/invalid/aggregates/test_flexible_array_only_member__1.c`: Struct with only a FAM → error (must have at least one named member).
- Added `test/invalid/aggregates/test_flexible_array_not_last__1.c`: FAM followed by another member → error (must be last).

### Step 7: Documentation and Version

- Updated `src/version.c` to set stage to 13400000.
- Updated `README.md` "Through stage" line to reference Stage 134 (bit-field and flexible array members in structs).
- Updated struct/union feature bullet in `README.md` to mention bit-field members and flexible array members.
- Removed "bit-fields" from "Not yet supported" section in `README.md`.
- Updated test totals in `README.md` to reflect 1946 passing tests (1262 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- Updated `docs/grammar.md` to add bit-field declarator syntax under `<struct_declarator>`: two forms (named with `:width`, anonymous with just `:width`).
- Updated `docs/self-compilation-report.md` with Stage 134 self-hosting result (C0→C1→C2 all pass).

## Final Results

All 1946 tests pass (1262 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with all 1946 tests passing at every stage; no source changes needed during bootstrap.

## Session Flow

1. Read stage spec and supporting documentation (templates and generated-artifacts reference).
2. Reviewed the reduced examples and identified the two bugs (CC99-006 and CC99-007).
3. Examined the current struct declarator parsing logic in `src/parser.c`.
4. Planned implementation: bit-field packing strategy, FAM validation rules, codegen paths for rvalue and lvalue.
5. Implemented Step 1 (type system): added StructField metadata fields.
6. Implemented Step 2 (parser bit-field logic): bit-field detection, constant-expression evaluation, storage-unit packing.
7. Implemented Step 3 (parser FAM logic): unsized-array validation, storage exclusion, "last member" check.
8. Implemented Step 4 (codegen rvalue reads): shift, mask, and load for both MEMBER_ACCESS and ARROW_ACCESS.
9. Implemented Step 5 (codegen lvalue writes): read-modify-write sequences for bit-field assignments.
10. Added 7 new test cases covering both features and validation errors.
11. Built and ran full test suite; verified all 1946 tests pass.
12. Verified self-host C0→C1→C2 bootstrap with no source changes.
13. Updated documentation (README.md, grammar.md, version.c, self-compilation-report.md).
14. Generated artifacts (milestone, transcript, kickoff checklist).
