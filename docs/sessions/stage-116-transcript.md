# stage-116 Transcript: Global Struct Array BSS Fix and Char[N] String-Literal Initialization

## Summary

Stage 116 fixes two silent codegen bugs affecting global (file-scope) struct and union arrays. Bug 1 causes uninitialized struct arrays to reserve too little BSS memory — the directive emitted `resd N` (4 × element count) instead of the full byte size needed for the array. Bug 2 prevents string literals in `char[N]` sub-arrays and struct fields from emitting inline bytes; instead an 8-byte pointer is emitted, corrupting memory layout. Both bugs were uncovered during the CC99-BENCH-002 benchmark bring-up effort. The fixes are confined to the code generator: no tokenizer, parser, or AST changes. After these fixes, the compiler correctly handles all global struct array patterns — uninitialized BSS allocation, initialized with struct-literal brace lists, and embedded `char[N]` fields initialized from string literals.

## Changes Made

### Step 1: Code Generator — Add emit_string_as_bytes() helper

- Added static helper function `emit_string_as_bytes(CodeGen *cg, ASTNode *str, int field_len)` in `src/codegen.c` just before `emit_global_array_elements()`.
- Emits a string literal's bytes inline as `db` directives, zero-padded to `field_len` bytes.
- Used in three sites to avoid duplicating the zero-padding logic: global array elements, recursive array-element helpers, and struct fields.
- Function body: iterate `byte_length` bytes from `str->value[]` as `db` directives, then emit zero bytes to fill the field width.

### Step 2: Code Generator — Bug 1a: codegen_emit_bss() single-dimension fix

- Modified `codegen_emit_bss()` single-dimension else-branch (~line 6877) inside the `gv->kind == TYPE_ARRAY` block.
- Changed from `fprintf(..., "%s: %s %d\n", gv->name, bss_res_directive(gv->full_type->base->kind), gv->full_type->length)` to `fprintf(..., "%s: resb %d\n", gv->name, gv->full_type->size)`.
- Fixes struct/union array BSS sizing: for `struct Point pts[100]` where `sizeof(Point) == 8`, now emits `pts: resb 800` (correct) instead of `pts: resd 100` (undersized to 400 bytes).
- Scalar arrays (`int arr[N]`, `char arr[N]`) now also emit `resb total` instead of element-directive × count; semantically identical but consistent.

### Step 3: Code Generator — Bug 1b: codegen_emit_local_statics() single-dimension fix

- Modified `codegen_emit_local_statics()` single-dimension else-branch (~line 7144) in the BSS section.
- Changed from `fprintf(..., "%s: %s %d\n", sv->label, bss_res_directive(sv->full_type->base->kind), sv->full_type->length)` to `fprintf(..., "%s: resb %d\n", sv->label, sv->full_type->size)`.
- Applies identical fix for block-scope static struct arrays: `static struct Pair data[100];` now reserves full `100 × sizeof(Pair)` bytes.

### Step 4: Code Generator — Bug 2a: codegen_emit_data() global array AST_STRING_LITERAL branch

- Modified `codegen_emit_data()` global array data-emission loop AST_STRING_LITERAL branch (~line 6802).
- Added type check: if `elem_type->kind == TYPE_ARRAY && elem_type->base->kind == TYPE_CHAR`, call `emit_string_as_bytes(cg, elem, elem_type->length)` to emit bytes inline.
- Otherwise, use the original pointer-pool path: add string to pool and emit `dq Lstr<N>`.
- Fixes `char rows[N][C] = {"hello", "world"}` (2-D char array) initialization; each row now emits as `C` inline bytes instead of an 8-byte pointer.

### Step 5: Code Generator — Bug 2b: emit_global_array_elements() recursive helper fix

- Modified `emit_global_array_elements()` catch-all before `compile_error` (~line 6550).
- Added new branch: `else if (elem_type && elem_type->kind == TYPE_ARRAY && elem_type->base->kind == TYPE_CHAR && elem->type == AST_STRING_LITERAL) { emit_string_as_bytes(cg, elem, elem_type->length); }`.
- Fixes the recursive case when nested-brace initializers are used in deeper array nesting levels.
- Example: `char arr[2][3][8] = {{"hello", "ok"}, {"a", "b"}}` now works at any nesting depth.

### Step 6: Code Generator — Bug 2c: emit_global_struct() struct field initialization fix

- Modified `emit_global_struct()` field-emission loop (~line 6655).
- Added new branch after the existing pointer-string case: `else if (f->kind == TYPE_ARRAY && f->full_type && f->full_type->base && f->full_type->base->kind == TYPE_CHAR && elem->type == AST_STRING_LITERAL) { emit_string_as_bytes(cg, elem, f->full_type->length); }`.
- Fixes struct fields like `char name[32]` initialized from string literals in global struct instances.
- Example: `struct Entry e = {1, "hello"};` where `e.name` is `char[32]` now works correctly.

### Step 7: Version

- Bumped `VERSION_STAGE` in `src/version.c` to `"01160000"`.

### Step 8: Tests

- Added 7 new valid test files in appropriate category subdirectories:
  - `test/valid/arrays/test_global_struct_array_bss__10.c` — global struct array BSS allocation test
  - `test/valid/arrays/test_global_struct_array_bss_large__42.c` — larger struct array BSS test
  - `test/valid/arrays/test_static_struct_array_bss__7.c` — block-scope static struct array BSS test
  - `test/valid/arrays/test_global_struct_array_init__10.c` — initialized global struct array test
  - `test/valid/arrays/test_global_char2d_string_init__65.c` — 2-D char array string initialization test
  - `test/valid/arrays/test_global_char2d_row_access__119.c` — 2-D char array row access test
  - `test/valid/structs/test_struct_char_array_field_init__104.c` — struct with char[N] field initialized from string
- Updated expected file: `test/print_asm/test_stage_22_01_global_bss_layout.expected` to reflect new `resb` directive for single-dimension arrays (was `resd`, now `resb`).

### Step 9: Self-hosting

- Executed `./build.sh --mode=self-host` (C0→C1→C2 cycle).
- C0 (GCC-built): all 1857 tests pass.
- C1 (C0-bootstrapped): all 1857 tests pass.
- C2 (C1-bootstrapped): all 1857 tests pass.
- No source changes needed during bootstrap; compiler's own source does not use global uninitialized struct arrays or the char[N] string-init pattern.
- Updated `docs/self-compilation-report.md` with stage-116 results.

## Final Results

All 1857 tests pass at C0, C1, and C2 (1175 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit). No regressions. Self-hosting cycle completes successfully with no source modifications required.

## Session Flow

1. Read stage spec to understand two bugs (BSS sizing for struct arrays, string-literal handling in char[N] contexts).
2. Reviewed `codegen_emit_bss()`, `codegen_emit_local_statics()`, `codegen_emit_data()`, `emit_global_array_elements()`, and `emit_global_struct()` functions to identify affected code paths.
3. Designed `emit_string_as_bytes()` helper to centralize string-as-bytes emission logic and avoid duplication across three sites.
4. Applied Bug 1a fix to `codegen_emit_bss()` single-dimension else-branch: changed from element-directive × count to `resb total_size`.
5. Applied Bug 1b fix to `codegen_emit_local_statics()` single-dimension else-branch: identical change for static arrays.
6. Applied Bug 2a fix to `codegen_emit_data()` global array AST_STRING_LITERAL branch: added type check for `char[N]` element type.
7. Applied Bug 2b fix to `emit_global_array_elements()` catch-all: added `char[N]-from-string` branch before compile_error.
8. Applied Bug 2c fix to `emit_global_struct()` field handler: added `char[N]-from-string` branch for struct fields.
9. Bumped version in `src/version.c` to `"01160000"`.
10. Built compiler and ran `test/valid/run_tests.sh` to verify existing tests still pass.
11. Created 7 new valid test files covering all fixed patterns: struct array BSS (global and static), struct array initialization, 2-D char array string init, and struct with char[N] field.
12. Updated expected file `test_stage_22_01_global_bss_layout.expected` to reflect new `resb` directive.
13. Ran full test suite `./test/run_all_tests.sh` — all 1857 tests pass.
14. Executed self-hosting cycle `./build.sh --mode=self-host` — C0→C1→C2 all pass with no compiler modifications.
15. Updated `docs/self-compilation-report.md` with stage-116 checkpoint results.

## Notes

The Bug 1 fix changes single-dimension array BSS emission for all types, not just structs. Integer arrays like `int arr[100]` now emit `resb 400` instead of `resd 100` (semantically identical: both reserve 400 bytes). This provides consistency across all array types and eliminates the type-specific dispatch. The `bss_res_directive()` function remains in the codebase because it is still used for multidimensional arrays and is not removed in this stage.

The Bug 2 fix adds three parallel paths for char[N] string initialization: in global arrays, in recursive array-element emission, and in struct fields. The helper function centralizes the byte-emission logic with zero-padding to avoid duplicating the pattern across these sites.

The compiler's own source does not use global uninitialized struct arrays (it uses heap-allocated storage within typed pointers), nor does it use the char[N] string-init pattern (it uses char* pointers to string literals). Therefore, bootstrap behavior is unaffected and no source modifications are needed during the self-hosting cycle.
