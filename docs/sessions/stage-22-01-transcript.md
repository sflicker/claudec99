# stage-22-01 Transcript: File-scope Uninitialized Object Declarations

## Summary

stage-22-01 adds support for file-scope (global) uninitialized object declarations. Globals are stored in the `.bss` section with zero-initialization and accessed via RIP-relative addressing. The implementation introduces a global symbol table that remains constant across function scopes, allowing multiple functions to read and write shared global state. Local variables shadow globals naturally through a lookup-order rule: the parser and codegen check the local scope first, then fall back to globals.

Key semantics: globals of all scalar types (char, short, int, long) and pointer/array types are supported. Storage is allocated with NASM pseudo-instructions (`resb`, `resw`, `resd`, `resq`) corresponding to object size. Code accesses globals via `[rel name]` (RIP-relative) addressing for position-independent code.

## Changes Made

### Step 1: Parser

- Updated `parse_external_declaration` to handle the `is_array` flag from declarators, ensuring file-scope array declarations set `TYPE_ARRAY` in full_type and track element count.
- Fixed missing logic that prevented parsing of global array declarations like `int arr[10];` at the translation unit level.

### Step 2: Codegen Header

- Added `MAX_GLOBALS` constant (64) to limit the global variable table.
- Defined `GlobalVar` struct with fields: `name`, `size`, `kind` (scalar/array/pointer), and `full_type` (for type checking and sizeof).
- Extended `CodeGen` struct with `globals[]` array and `global_count` to track registered global variables.

### Step 3: Codegen Implementation

- Added `codegen_find_global` to search the global table by name.
- Added `global_var_type` to retrieve the full_type of a global.
- Added `emit_load_global` to emit RIP-relative load instructions.
- Added `emit_store_global` to emit RIP-relative store instructions.
- Added `codegen_add_global` to register a new global with size and type information.
- Added `bss_res_directive` helper to map object size to the correct NASM pseudo-instruction.
- Added `codegen_emit_bss` to iterate the global table and emit `.bss` section directives.
- Modified `codegen_init` to zero `global_count`.
- Updated `sizeof_type_of_expr` to add global fallbacks for AST_VAR_REF, AST_DEREF, AST_ARRAY_INDEX, and AST_ASSIGNMENT when no local variable is found.
- Updated `expr_result_type` to add global fallback for AST_VAR_REF.
- Updated `codegen_expression` to emit RIP-relative loads for globals in AST_VAR_REF and use global stores in AST_ASSIGNMENT. Added global-aware codegen for AST_ADDR_OF, AST_PREFIX_INC_DEC, AST_POSTFIX_INC_DEC, and AST_SIZEOF_EXPR.
- Updated `emit_array_index_addr` to handle global arrays and global pointers with RIP-relative base addressing.
- Modified `codegen_translation_unit` to register global variables before processing function definitions and emit `.bss` section before `.text`.

### Step 4: Documentation

- Updated `docs/grammar.md` restriction comment from "no file-scope variable declarations" to "file-scope object declarations must be uninitialized", reflecting the new capability.

### Step 5: Tests

- Added 9 valid tests covering:
  - Basic global declaration and access (`test_global_basic__7.c`).
  - Local variable shadowing of globals (`test_global_shadow__3.c`).
  - Globals persisting across function calls (`test_global_persist__2.c`).
  - Type coverage: char, short, int, long (`test_global_types__42.c`).
  - Pointer globals (`test_global_pointer__5.c`).
  - Array globals (`test_global_array__9.c`).
  - Zero-initialization verification (`test_global_zero_init__0.c`).

- Added 2 print_asm tests with `.expected` files:
  - `test_stage_22_01_global_bss_layout.c`: verifies `.bss` section structure.
  - `test_stage_22_01_global_rip_relative.c`: verifies RIP-relative addressing patterns.

- Added 1 invalid test:
  - `test_invalid_109__duplicate_global_declaration.c`: ensures duplicate global declarations are rejected.

## Final Results

All 645 tests pass (up from 635 before stage-22-01).

- valid: 401 passed (up from 394)
- invalid: 111 passed (up from 110)
- print_asm: 21 passed (up from 19)
- print_ast: 66 passed
- print_tokens: 46 passed

No regressions. Build successful.

## Session Flow

1. Reviewed spec and identified requirements (global symbol table, .bss emission, RIP-relative addressing, local shadowing).
2. Examined existing parser and codegen to understand how local variables are handled.
3. Designed global symbol table structure and integration points.
4. Implemented parser fix for array declarations at file scope.
5. Implemented codegen header changes (GlobalVar struct, global_count).
6. Implemented codegen support: global registration, load/store helpers, .bss emission, expression fallbacks.
7. Added comprehensive test suite (9 valid, 2 print_asm, 1 invalid).
8. Ran full test harness; all tests pass.
9. Updated docs/grammar.md to reflect new capability.
10. Generated milestone summary and transcript.

## Notes

- **Duplicate global check**: Not required by spec but added for correctness (C does not permit duplicate definitions at file scope).
- **Local-first lookup**: The codegen checks local scope before global scope, ensuring natural shadowing semantics.
- **Section order**: `.bss` is emitted before `.text` to follow conventional ELF section ordering.
- **Spec typo**: The spec lists `arr; resd 10` but the correct NASM syntax is `arr: resd 10` (colon, not semicolon). Implementation uses the correct syntax.
