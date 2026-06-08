# stage-95-10 Transcript: Remove static char arrays from parser.h

## Summary

Stage 95-10 continues the fixed-capacity reduction work by converting seven `char field[MAX_NAME_LEN]` embedded arrays in parser.h structs to `const char *` pointers. The fields converted are: `EnumConst.name`, `EnumTag.tag`, `StructTag.tag`, `UnionTag.tag`, `TypedefEntry.name`, `FuncSig.name`, and `GlobalObjSig.name`. Each conversion replaces `strncpy` calls with direct pointer assignments, since all name and tag values derive from `parser->current.value`, which is a persistent pointer into the lexer string pool (established in stage 95-08). Three local `char[256]` temporary buffers used during struct, union, and enum specifier parsing are also simplified to `const char *` pointers. The stage completes parser.h struct field elimination, removing the `MAX_NAME_LEN` cap from all parser.h name/tag fields.

## Changes Made

### Step 1: TypedefEntry.name conversion

- Changed `TypedefEntry.name` from `char name[MAX_NAME_LEN]` to `const char *name` in include/parser.h.
- Updated `parser_register_typedef` in src/parser.c to assign `entry.name = name;` instead of using `strncpy`.
- Verified pointer safety: `name` parameter comes from `parser->current.value` (lexer pool pointer).
- All tests pass; committed.

### Step 2: FuncSig.name conversion

- Changed `FuncSig.name` from `char name[MAX_NAME_LEN]` to `const char *name` in include/parser.h.
- Updated `parser_register_function` in src/parser.c to assign `sig.name = name;` instead of using `strncpy`.
- All tests pass; committed.

### Step 3: GlobalObjSig.name conversion

- Changed `GlobalObjSig.name` from `char name[MAX_NAME_LEN]` to `const char *name` in include/parser.h.
- Updated `parser_register_global` in src/parser.c to assign `sig.name = name;` instead of using `strncpy`.
- All tests pass; committed.

### Step 4: StructTag.tag conversion

- Changed `StructTag.tag` from `char tag[MAX_NAME_LEN]` to `const char *tag` in include/parser.h.
- Updated `parser_register_struct_tag` in src/parser.c to assign `st.tag = tag;` instead of using `strncpy`.
- Simplified local `char[256]` buffer in `parse_struct_specifier` to `const char *tag` assigned directly from `parser->current.value`.
- Replaced two-step `strncpy` sequence with single pointer assignment.
- All tests pass; committed.

### Step 5: UnionTag.tag conversion

- Changed `UnionTag.tag` from `char tag[MAX_NAME_LEN]` to `const char *tag` in include/parser.h.
- Eliminated two-step push-then-strncpy pattern in `parser_register_union_tag`; replaced with single pointer assignment.
- Simplified local `char[256]` buffer in `parse_union_specifier` to `const char *tag` assigned directly from `parser->current.value`.
- All tests pass; committed.

### Step 6: EnumTag.tag conversion

- Changed `EnumTag.tag` from `char tag[MAX_NAME_LEN]` to `const char *tag` in include/parser.h.
- Converted inline enum tag registration in `parse_enum_specifier` from buffered approach to direct pointer assignment.
- Simplified local `char[256]` buffer to `const char *tag` assigned directly from `parser->current.value`.
- All tests pass; committed.

### Step 7: EnumConst.name conversion

- Changed `EnumConst.name` from `char name[MAX_NAME_LEN]` to `const char *name` in include/parser.h.
- Simplified local `char ename[256]` buffer in `parse_enum_specifier` to `const char *ename` assigned directly from `parser->current.value`.
- Replaced `strncpy` with direct pointer assignment in enum constant registration.
- All tests pass; committed.

### Step 8: Documentation updates

- Updated `docs/fixed-capacity-inventory.md` to mark parser.h struct name/tag fields completed in stage 95-10 and note removal of `MAX_NAME_LEN` limit from all parser.h fields.
- Updated `src/version.c` to stage 00951000.
- Verified self-host: C0→C1→C2 all pass 1479/1479 tests without bootstrap failures.

## Final Results

All 1479 tests pass (165 unit, 828 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). No regressions. Self-hosting verification (C0→C1→C2 cycle) confirms all stages compile and execute correctly with no bootstrap failures.

## Session Flow

1. Read spec and context (fixed-capacity inventory, prior stage conversions).
2. Reviewed parser.h structs and identified 7 fields with embedded `char[MAX_NAME_LEN]` arrays.
3. Verified pointer safety: all field values come from `parser->current.value` (lexer string pool pointer since stage 95-08).
4. Implemented conversions in sequence, one field per commit (7 commits + 1 docs commit).
5. Ran full test suite after each field conversion to ensure 1479/1479 pass.
6. Updated `docs/fixed-capacity-inventory.md` and version constant.
7. Verified self-hosting with C0→C1→C2 cycle; all stages pass.
