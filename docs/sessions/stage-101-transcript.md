# stage-101 Transcript: Block-Scope Static Arrays and Structs

## Summary

Stage 101 enables block-scope `static` local variables to hold aggregate types (arrays, structs, and unions) in addition to scalars and pointers. The feature was previously rejected at compilation time, but all the infrastructure to support it—label generation, static-variable registration, RIP-relative addressing, and `.data`/`.bss` emission—was already in place from stages 71 and later. This stage removes the guard and extends four codegen sites to handle `is_static` branches for aggregates. Initialized arrays and structs can be populated from brace-list initializers; char arrays can also be initialized from string literals. Uninitialized statics are zero-filled in `.bss`. All patterns persist correctly across function calls and compile to efficient RIP-relative addressing.

## Changes Made

### Step 1: AST and Header Changes

- Added `ASTNode *init_node` field to `LocalStaticVar` struct in `include/codegen.h` (lines 35–44). This field carries the `AST_INITIALIZER_LIST` node (or `AST_STRING_LITERAL` for char arrays) when an aggregate static is initialized; it is `NULL` for uninitialized statics and for scalars.

### Step 2: Remove Guard and Register Array/Struct Statics in codegen_statement

- In `src/codegen.c`, `codegen_statement` SC_STATIC arm (lines ~4279–4335):
  - Deleted the explicit guard that rejected `TYPE_ARRAY`, `TYPE_STRUCT`, and `TYPE_UNION`.
  - Added static-array registration block (after existing scalar block) that validates initializers, generates `Lstatic_<func>_<n>` label, registers `LocalVar` with `is_static=1` and `offset=0`, and pushes `LocalStaticVar` entry with `init_node` set to the initializer (if present) or `NULL`.
  - Added static-struct/union registration block (same pattern as array) that checks for complete types, validates initializers, and registers both `LocalVar` and `LocalStaticVar` with `is_static=1`.
  - Both blocks validate that initializers (if present) are `AST_INITIALIZER_LIST` (or `AST_STRING_LITERAL` for char arrays); non-brace initializers are rejected with compile error.

### Step 3: Extend codegen_emit_local_statics for Aggregate Emission

- In `src/codegen.c`, `codegen_emit_local_statics` function (lines ~5750–5776):
  - **`.data` loop (initialized statics):** Added conditional branches before scalar fallthrough:
    - Char-array-from-string-literal: emits `db` directive with byte values, padding to full array length.
    - Array-from-brace-list: reuses slots-mapping pattern from `codegen_emit_data` (compute length, build slots array, walk initializer children handling `AST_DESIGNATED_INIT`, emit each slot).
    - Struct-from-brace-list: calls `emit_global_struct` to emit field values.
    - Union-from-brace-list: inlines first-member initialization logic.
  - **`.bss` loop (uninitialized statics):** Replaced single line with conditional:
    - Array: emits `<res-directive> <length>` (e.g., `resd 8` for 8-element int array).
    - Struct/union: emits `resb <size>`.
    - Scalar fallthrough: uses `<res-directive> 1`.

### Step 4: Array Decay in codegen_expression VAR_REF

- In `src/codegen.c`, `codegen_expression` AST_VAR_REF arm (lines ~2068–2073):
  - For `lv->kind == TYPE_ARRAY`, added `is_static` check:
    - If static: `lea rax, [rel <label>]`
    - If not static: `lea rax, [rbp - <offset>]`

### Step 5: Array Subscript Base in emit_array_index_addr

- In `src/codegen.c`, `emit_array_index_addr` local-array branch (lines ~985–989):
  - Removed stale comment claiming array statics are never reached.
  - Added `is_static` check:
    - If static: `lea rax, [rel <label>]`
    - If not static: `lea rax, [rbp - <offset>]`

### Step 6: Struct Member Address in emit_member_addr

- In `src/codegen.c`, `emit_member_addr` local-struct VAR_REF branch (lines ~1177–1192):
  - Replaced the unconditional stack-relative `lea rax, [rbp - <offset>]` with:
    - If static: `lea rax, [rel <label>]` (or `[rel <label> + <f->offset>]` if member offset is nonzero)
    - If not static: `lea rax, [rbp - (<offset> - <f->offset>)]` (existing logic)

### Step 7: Version Bump

- In `src/version.c`, bumped `VERSION_STAGE` from `"01000000"` to `"01010000"`.

### Step 8: Test Suite

- Added 6 valid tests:
  - `test_block_static_array_uninitialized__0.c` — uninitialized int array persists across calls (checked via increment).
  - `test_block_static_array_initialized__0.c` — initialized int array from brace-list.
  - `test_block_static_struct_uninitialized__0.c` — uninitialized struct persists.
  - `test_block_static_struct_initialized__0.c` — initialized struct from brace-list.
  - `test_block_static_char_array_string__0.c` — char array initialized from string literal.
  - `test_block_static_array_persists__0.c` — single-element array as counter across three calls.
- Added 2 invalid tests:
  - `test_block_static_array_nonconstant_init__brace.c` — non-brace initializer for static array (error).
  - `test_block_static_struct_nonconstant_init__brace.c` — non-brace initializer for static struct (error).

## Final Results

All 1552 tests pass (880 valid, 250 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). No regressions.

Self-host C0→C1→C2 cycle completes cleanly; no changes to compiler source code were needed. C2 is identical to C1, confirming fixed-point stability.

## Session Flow

1. Read stage 101 spec and reviewed reference templates (milestone-summary-format.md, transcript-format.md).
2. Examined current README.md and docs/grammar.md to understand update scope.
3. Reviewed implementation summary provided by user: guard removal, Task 1–6 codegen edits, version bump, test additions, and test results.
4. Verified test totals and self-host status from the completed stage.
5. Generated milestone summary (concise subsystem-oriented format).
6. Generated transcript (structured summary of changes, test additions, and final results).
7. Prepared to update README.md and docs/grammar.md as part of artifact completion.
