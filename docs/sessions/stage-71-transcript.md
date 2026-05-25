# stage-71 Transcript: Block-Scope Static Variables

## Summary

Stage 71 adds support for block-scope `static` variables, a C99 feature that allows local variables to retain their values across function calls. Block-scope statics have static storage duration (allocated once at program startup), are zero-initialized if no initializer is provided, and must have constant initializers if initialized. The implementation stores these variables in the .bss segment (uninitialized) or .data segment (initialized), addressing them via RIP-relative labels rather than stack offsets.

Semantically, block-scope statics respect the scoping and shadowing rules of their enclosing block—when a block exits, the static entry is removed from scope, but the backing storage persists and is reused on the next call. Only scalar and pointer types are supported; arrays and structs at block scope are out of scope. The `extern` storage class specifier remains unsupported at block scope.

## Changes Made

### Step 1: Parser

- Modified `parse_statement` to recognize `TOKEN_STATIC` at block scope (previously rejected with "storage class specifier not allowed").
- When `static` is encountered in statement position, the parser consumes the keyword and recursively invokes `parse_declaration` to parse the declarator and optional initializer.
- The resulting `Decl` node is marked with `SC_STATIC` storage class (enum StorageClass).
- Storage class validation rejects `extern` at block scope (the old error message is preserved for that case); `static` is now valid.

### Step 2: Code Generator — Data Structures

- Extended `LocalVar` struct in `include/codegen.h`:
  - Added `is_static: 1` bitfield to mark static locals.
  - Added `static_label[256]` buffer to store the unique label string (e.g., "Lstatic_func_0").
- Added `LocalStaticVar` struct to track block-scope static variables in the backing pool:
  - Fields: `name`, `func_name`, `type`, `has_initializer`, `init_value`.
- Extended `CodeGen` struct:
  - Added `local_statics[MAX_LOCAL_STATICS]` pool to hold local static metadata.
  - Added `local_static_count` counter.

### Step 3: Code Generator — Static Variable Declaration

- Modified `codegen_statement` to handle `SC_STATIC` declarations (when `stmt->is_declaration && var->storage_class == SC_STATIC`).
- Validates that the initializer, if present, is a constant expression (int literal, char literal, unary minus + int literal); non-constant initializers produce a compile error.
- Rejects static arrays and static structs with "not yet supported" error.
- Generates a unique label using `sprintf(label, "Lstatic_%s_%d", func_name, cg->label_count++)`.
- Registers the static in the `local_statics` pool with metadata (name, func_name, type, initializer info).
- Adds the variable to `cg->locals[]` array with `is_static=1` so that scope tracking and shadowing work via the existing `codegen_find_var` lookup mechanism.
- Does not allocate stack space for the static variable.

### Step 4: Code Generator — Variable Access

- Updated all code paths that access local variables to check `var->is_static`:
  - **Load** (`codegen_expression` for variables): If static, emit `mov eax, [rel Lstatic_func_N]` (or appropriate register/size).
  - **Store** (`codegen_assignment`): If static, emit `mov [rel Lstatic_func_N], eax`.
  - **Address-of** (`codegen_unary_addressof`): If static, emit `lea eax, [rel Lstatic_func_N]`.
  - **Prefix/postfix increment/decrement**: Load from static, modify, store to static.
  - **Subscript** (`codegen_subscript`): If the base is a static pointer or array, use RIP-relative addressing for the load.
  - **Member access** (`codegen_member_access` and `codegen_arrow_access`): If the struct is a static, use RIP-relative addressing for the load.

### Step 5: Code Generator — Static Emission

- Added `codegen_emit_local_statics()` function, called after string pool emission and before code section emission.
- Iterates over the `local_statics` pool and emits each static to .bss or .data:
  - If no initializer: emit to .bss with `label: resd size` (or resq/resb as appropriate for the type).
  - If initializer: emit to .data with `label: dd init_value` (or dq/db as appropriate).
- Label format: `Lstatic_<func>_<N>` (not `.Lstatic` — NASM local labels scoped to the preceding non-local label would cause linker errors when the static is defined in .bss and referenced from code in other functions).

### Step 6: Tests

- **test_block_static_persist__3.c**: Basic persistence—function increments a static int and returns the new value; called twice in main, sum is 3.
- **test_block_static_init__83.c**: Explicit constant initializer—function increments a static initialized to 40, returns new value; called twice, sum is 83.
- **test_block_static_separate_funcs__33.c**: Two functions with separate static int variables; each maintains its own counter across calls.
- **test_block_static_nested__3.c**: Static variable declared in an if-block; persists across calls and respects block scope.
- **test_block_static_nonconstant__constant_expression.c**: Invalid test—attempts to initialize a static with a non-constant expression (variable); compiler rejects with error.
- **test_invalid_118__storage_class_specifier_not_allowed.c**: Updated—changed from `static int x = 1;` (now valid) to `extern int x;` (still invalid at block scope).

## Final Results

Build succeeds. All 1148 tests pass (709 valid, 213 invalid, 67 integration, 39 print-AST, 99 print-tokens, 21 print-asm). No regressions. Full suite up from 1143 (stage 70.03) to 1148 (5 new tests).

## Session Flow

1. Read spec and supporting template files.
2. Reviewed existing code: LocalVar, codegen statement handling, variable access paths.
3. Planned implementation: extend LocalVar with is_static flag and static_label; add LocalStaticVar pool; modify parser to allow static at block scope; update all variable access paths; add static emission phase.
4. Implemented parser changes to accept static at block scope and mark declarations with SC_STATIC.
5. Implemented codegen data structures and static declaration handling.
6. Implemented static variable access (load, store, address-of, inc/dec, subscript, member access) with RIP-relative addressing.
7. Implemented static emission function and integrated into codegen pipeline.
8. Ran full test suite; verified 5 new tests pass and no regressions.
9. Updated README.md and grammar.md to reflect new feature and updated test totals.
10. Generated milestone summary and transcript artifacts.
