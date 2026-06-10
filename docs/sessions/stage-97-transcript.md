# stage-97 Transcript: Designated Initializers

## Summary

Stage 97 implements C99 designated initializers — the `.member = value` and `[index] = value` syntax forms that enable brace-initializer lists to specify target fields or array elements by name or index instead of relying on positional order. This allows sparse initialization (e.g., `int a[10] = {[5] = 99}`), out-of-order field initialization (e.g., `struct Point p = {.y = 10, .x = 3}`), and mixed designated-and-plain initialization where subsequent plain elements advance from the designated position. All initializers in arrays and struct members are zero-filled unless explicitly set.

The implementation adds the `AST_DESIGNATED_INIT` node type, extends the parser to detect and encode designators, and rewrites four code-generation paths (local struct, local array, global struct, global array) to use a cursor or slots-mapping pattern to handle non-positional element/field assignment. Chained designators (e.g., `.a.b` or `.arr[2]`) are detected and rejected as out-of-scope. The compiler self-hosts without issue; all 1501 tests pass.

## Changes Made

### Step 1: AST Node Definition

- Added `AST_DESIGNATED_INIT` to `ASTNodeType` enum in `include/ast.h` (after `AST_INITIALIZER_LIST`).
- Designated node structure: `value != NULL` indicates a member designator (string with field name); `value == NULL` indicates an array index designator (index stored in `byte_length`); always exactly one child (the initializer value, which may itself be a nested list).

### Step 2: Parser — `parse_initializer_element`

- Added forward declaration of `eval_case_const_expr` at the top of `src/parser.c` (before functions that reference it).
- Implemented new static helper `parse_initializer_element()` to parse one initializer-list element: a designator (if present) followed by `=` and a value, or a plain initializer.
- Member designator path: consume `.`, extract identifier name, expect `=`, check for chained designators (error if another `.` or `[` follows before the `=` is consumed), create `AST_DESIGNATED_INIT` node with name in `value` field.
- Array index designator path: consume `[`, evaluate constant integer expression via `eval_case_const_expr`, validate non-negative, expect `]` and `=`, check for chained designators, create `AST_DESIGNATED_INIT` node with `NULL` value and index in `byte_length`.
- Plain initializer fallthrough: return result of `parse_initializer()`.
- Updated `parse_initializer` loop (inside the `TOKEN_LBRACE` branch) to call `parse_initializer_element()` for each list element instead of `parse_initializer()` directly.
- Bug fix: chained-designator error check positioned before `parser_expect(TOKEN_ASSIGN)` so the error message correctly reports "chained designators not yet supported" instead of "expected '=', got '.'".

### Step 3: AST Pretty Printer

- Added `case AST_DESIGNATED_INIT:` to `src/ast_pretty_printer.c`.
- For member designators (value != NULL): output `DESIGNATED_INIT(.fieldname)` followed by the child on the next indentation level.
- For array designators (value == NULL): output `DESIGNATED_INIT([N])` (where N is the index from `byte_length`) followed by the child.

### Step 4: Codegen — Local Struct Initialization

- Rewrote `emit_local_struct_init()` in `src/codegen.c` to support a `cur_field` cursor tracking the current sequential position.
- For each child in the initializer list:
  - If `AST_DESIGNATED_INIT` with `value != NULL` (member designator): perform member-name lookup to find the target field index; if not found, emit a compile error.
  - If `AST_DESIGNATED_INIT` with `value == NULL` (array designator): emit error "array index designator in struct initializer".
  - If plain child: use current cursor position.
  - Advance `cur_field` after each initializer; bounds-check that `cur_field < field_count`.
- The struct is already zero-filled before this function is called; the function only overwrites fields with explicit initializers.

### Step 5: Codegen — Local Array Initialization

- Replaced the positional loop in `codegen_statement` (inside `AST_DECLARATION` / `TYPE_ARRAY` branch) with a two-phase approach:
  - **Phase 1 (zero-fill)**: Unconditionally emit `mov byte [rbp - offset], 0` for each byte in the array, regardless of whether designators are present.
  - **Phase 2 (apply initializers)**: Iterate the initializer list with a `cur` index cursor:
    - If `AST_DESIGNATED_INIT` with `value == NULL` (array designator): set `cur` to the index; bounds-check that index is within `[0, length)`.
    - If `AST_DESIGNATED_INIT` with `value != NULL` (member designator): emit error "member designator in array initializer".
    - If plain child: use current cursor position.
    - After applying each initializer, increment `cur`; bounds-check that `cur < length` before emission.
- Removed the early `list->child_count > length` check (which assumed positional correspondence) and the trailing zero-fill arm (now handled by Phase 1).

### Step 6: Codegen — Global Struct Initialization

- Rewrote `emit_global_struct()` in `src/codegen.c` with a slots-mapping pattern:
  - Allocate `ASTNode *slots[MAX_STRUCT_FIELDS_DESIGNATED]` (fixed-size for self-host compatibility; `MAX_STRUCT_FIELDS_DESIGNATED = 64` covers all compiler-internal structs).
  - First pass: walk the initializer list with a `cur_field` cursor, filling `slots[field_index]` for each element (designated or positional).
  - Second pass: iterate fields in declared order; for each field, emit its `slots` entry if non-NULL, or emit zero bytes if NULL.
  - This preserves natural alignment and padding while supporting out-of-order and sparse initialization.

### Step 7: Codegen — Global Array Initialization

- Extended `codegen_emit_data()` (global array initialization branch) with a slots-mapping pattern:
  - Allocate `ASTNode *slots[MAX_ARRAY_ELEMS_DESIGNATED]` (fixed-size for self-host; `MAX_ARRAY_ELEMS_DESIGNATED = 1024` covers all realistic compile-time tables).
  - First pass: walk the initializer list with a `cur` index cursor, filling `slots[index]` for each element (designated or positional).
  - Bounds-check both array designator indices and positional element count.
  - Second pass: iterate indices `0` to `length-1`; for each index, emit the element at `slots[i]` if non-NULL, or emit zero if NULL.
  - Error checking for out-of-bounds indices and mismatched designator types.

### Step 8: Version Bump

- Updated `src/version.c`: `VERSION_STAGE = "00970000"`.

### Step 9: Tests

- **10 valid tests**: `test_designated_init_local_struct_basic__13.c` (out-of-order fields), `test_designated_init_local_struct_partial__7.c` (sparse, zero-fill), `test_designated_init_local_struct_mixed__5.c` (mixed designated and plain), `test_designated_init_local_array_basic__30.c` (index designators), `test_designated_init_local_array_advance__11.c` (cursor advances), `test_designated_init_local_array_reorder__10.c` (reordered indices), `test_designated_init_global_struct__8.c` (global struct, out-of-order), `test_designated_init_global_array_sparse__9.c` (global array, sparse), `test_designated_init_array_of_structs__3.c` (index designator with nested struct), `test_designated_init_char_literal_index__65.c` (char literals as indices).
- **5 invalid tests**: `test_designated_init_unknown_member__struct_has_no_member_named.c`, `test_designated_init_array_index_in_struct__array_index_designator_in_struct.c`, `test_designated_init_member_in_array__member_designator_in_array.c`, `test_designated_init_array_index_oob__array_designator_index.c`, `test_designated_init_chained__chained_designators_not_yet_supported.c`.
- **2 print_ast tests**: `test_designated_init_member.c` + `.expected` (member designator output format), `test_designated_init_index.c` + `.expected` (array index designator output format). Note: `.expected` files written using bash `>` redirect to capture trailing whitespace correctly.
- **1 integration test**: `test_designated_init_multifile/` with two source files, a main file, and `run_test.sh`.

### Step 10: Documentation

- Updated `docs/grammar.md`: replaced the `<initializer_list>` and `<initializer>` rules with new grammar supporting `<initializer_element>` (optional designator + `=` + value, or plain initializer) and `<designator>` (`. IDENTIFIER` or `[ <constant_expression> ]`).
- Updated `README.md`: added "designated initializers (`[idx] = …`, `.member = …`)" to the feature summary under aggregate initialization; bumped test total from 1483 to 1501; updated breakdown: 843 valid, 242 invalid, 85 integration, 45 print_ast, 100 print_tokens, 21 print_asm.
- Removed "Designated initializers" from the "Not yet supported" section.
- Generated supplemental documents: `docs/self-compilation-report.md` (Stage 97 entry, no issues), `docs/outlines/checklist.md` (stage 97 section, all designated-initializer TODOs flipped to [x]), `docs/other/stage-97-parse-tree.md` (parse tree snapshot), `status/project-status-through-stage-97.md`, `status/project-features-through-stage-97.md`.

## Final Results

All 1501 tests pass at all three stages:
- C0 (GCC-built): 1501/1501 pass
- C1 (C0-built, self-host bootstrap): 1501/1501 pass
- C2 (C1-built, second self-host stage): 1501/1501 pass

Breakdown: 843 valid, 242 invalid, 85 integration, 45 print_ast, 100 print_tokens, 21 print_asm.

No regressions. No bootstrap bugs found during self-hosting. The compiler reached a fixed point — C1 and C2 are behavior-identical.

## Session Flow

1. Read spec and supporting template files; reviewed stage 96 milestone as a format reference.
2. Analyzed existing initializer machinery (positional `AST_INITIALIZER_LIST`, four codegen paths).
3. Implemented AST node (`AST_DESIGNATED_INIT`) and parser helper (`parse_initializer_element`) with forward declaration of `eval_case_const_expr`.
4. Added `print_ast` support for the new node type.
5. Rewrote `emit_local_struct_init` with cursor-pattern field tracking and member-name lookup.
6. Converted local array init to two-phase (zero-fill + apply with cursor).
7. Rewrote `emit_global_struct` and `codegen_emit_data` (array branch) with slots-mapping to support non-positional elements.
8. Bumped version.
9. Added 18 tests (10 valid, 5 invalid, 2 print_ast, 1 integration); verified pass/fail status.
10. Identified and fixed bug: moved chained-designator error check before `parser_expect(TOKEN_ASSIGN)`.
11. Fixed print_ast `.expected` output: used bash `>` redirect to preserve trailing whitespace.
12. Updated grammar and README.
13. Generated documentation artifacts (self-compilation report, checklist, parse tree, status/features snapshots).
14. Ran full test suite: all 1501 tests pass.
15. Executed `./build.sh --mode=self-host` for C0→C1→C2 verification: passed cleanly.
16. Committed changes.

## Notes

- **Chained designator error placement**: Initially placed the check after `parser_expect(TOKEN_ASSIGN)`, which meant parsing `.a.v = 1` would consume `.a`, find `=`, then report "expected '=', got '.'". Moved the check to before the `=` consumption to emit the correct diagnostic: "chained designators not yet supported".
- **Bootstrap self-host constraint**: All local arrays in codegen code use fixed-size constants (`MAX_STRUCT_FIELDS_DESIGNATED = 64`, `MAX_ARRAY_ELEMS_DESIGNATED = 1024`) instead of VLAs because the C0 parser cannot handle runtime-sized arrays.
- **Print_ast expected output**: The `.expected` files require trailing whitespace preservation; used bash `> file` redirect (not `>> file`) and ensured proper newline termination.
- **Slots mapping strategy**: Avoids re-scanning or reordering the initializer AST; instead builds a map and emits in the natural declared/sequential order, preserving alignment and padding semantics.

## Commit

The stage-97 implementation is captured in a single commit with all changes (parser, codegen, AST, version, tests, docs) and incremental self-host validation.
