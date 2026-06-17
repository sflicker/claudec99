# stage-135 Transcript: Type Compatibility and Composite Type Checks

## Summary

Stage 135 fixes two C99 function-parameter type compatibility bugs. **CC99-008 (array parameter adjustment)**: `int a[3]`, `int a[]`, and `int *a` declared as function parameters are now compatible. The fix applies C99 §6.7.5.3p7 array-to-pointer and §6.7.5.3p8 function-to-pointer-to-function adjustments in `parse_parameter_declaration` before redeclaration comparison occurs, so that `int f(int a[3])`, `int f(int a[])`, and `int f(int *a)` all produce the same adjusted parameter type. A pre-existing bug where `(void)` in a function-pointer parameter list was counted as one void parameter instead of zero is also fixed via a peek-ahead check in `parse_declarator`'s function-pointer parameter loop. **CC99-009 (pointer-to-array types)**: `int (*row)[4]` and `int (*row)[]` are now valid parameter declarations. `parse_declarator` no longer rejects the `(*name)[N]` pattern; instead, three new fields in `ParsedDeclarator` (`is_ptr_to_array`, `ptr_to_array_length`, `ptr_to_array_has_size`) track the array bounds, and `parse_parameter_declaration` builds the correct composite type `pointer(array(base, N))`. Indexed access `(*row)[i]` works via the existing codegen path, so no code generation changes are needed. The composite type rule applies the TypeKind-only compatibility check that permits `int (*row)[]` and `int (*row)[4]` to be compatible.

## Changes Made

### Step 1: Parser - Array-to-Pointer Parameter Adjustment

- In `parse_parameter_declaration`, after calling `parse_declarator` to get a `ParsedDeclarator d`, check if `d.is_array == 1` (declarator was an array form).
- When true, set `param->decl_type = TYPE_POINTER` and `param->full_type = type_pointer(base_type)` instead of using the array type.
- This ensures `int a[3]`, `int a[]`, and `int *a` all produce `TYPE_POINTER` with compatible full types before redeclaration checks.

### Step 2: Parser - Function-to-Pointer-to-Function Parameter Adjustment

- In `parse_parameter_declaration`, after the array-adjustment branch, add a check for `d.is_function == 1` (declarator was a function form).
- When true, the attached parameter list was already parsed by `parse_declarator`. Consume that list and set `param->decl_type = TYPE_POINTER` with `param->full_type = type_pointer(param_function_type)`.
- This ensures `int cb(void)` and `int (*cb)(void)` both adjust to pointer-to-function before comparison.

### Step 3: Parser - Fix (void) Parameter Count in Function Pointers

- In `parse_declarator`, within the function-pointer parameter loop (where we parse the parameters for `(*name)(params)`):
- Add a peek-ahead check before emitting a void parameter: if we see `void` followed by `)`, count it as zero parameters and skip the `(void)` → do not emit a parameter.
- Previously `(void)` was counted as one `TYPE_VOID` parameter; now it is correctly treated as "no parameters" per C99.

### Step 4: Parser - Support Pointer-to-Array Declarators

- In `parse_declarator`, remove the explicit error `"pointer to array types are not supported"` that was triggered when `(*name)` was followed by `[N]` or `[]`.
- Instead, add three new fields to `ParsedDeclarator`: `int is_ptr_to_array`, `int ptr_to_array_length`, `int ptr_to_array_has_size`.
- When we detect `(*name)` followed by `[`, parse the bracket content:
  - If `[N]` with a constant, store `is_ptr_to_array = 1`, `ptr_to_array_length = N`, `ptr_to_array_has_size = 1`.
  - If `[]` (unbound), store `is_ptr_to_array = 1`, `ptr_to_array_length = 0`, `ptr_to_array_has_size = 0`.

### Step 5: Parser - Build Pointer-to-Array Types in parse_parameter_declaration

- In `parse_parameter_declaration`, after `parse_declarator`, add a new branch checking `d.is_ptr_to_array == 1`.
- When true, build the array type first: `Type *arr_type = type_array(base_type, d.ptr_to_array_length)` (or length 0 if `!d.ptr_to_array_has_size`).
- Then wrap it in a pointer: `param->full_type = type_pointer(arr_type)` and set `param->decl_type = TYPE_POINTER`.

### Step 6: Codegen - No Changes Needed

- The existing `emit_array_index_addr` codegen path (from stage 28-04) correctly handles `(*ptr_to_array)[idx]` patterns as long as the local variable has the correct `full_type`.
- The array type nested inside the pointer allows the subscript operator to compute the correct element address without additional codegen logic.

### Step 7: Tests

- Added `test/valid/functions/test_array_param_compat_bracket3_ptr__9.c`: Declares `int a[3]`, `int a[]`, and `int *a` across three function redeclarations, returns 9 to verify compatibility.
- Added `test/valid/functions/test_array_param_unbound_ptr_compat__6.c`: Declares with `int a[]` then `int *a`, returns 6.
- Added `test/valid/functions/test_array_param_bound_to_unbound_compat__0.c`: Declares with `int a[3]` then `int a[]`, returns 0 (no error).
- Added `test/valid/functions/test_func_param_adjust_compat__0.c`: Declares `int f(int cb(void))` and `int f(int (*cb)(void))` as compatible, returns 0 (no error).
- Added `test/valid/pointers/test_pointer_to_array_param__13.c`: CC99-009 reduced example with `int tail(int (*row)[])` and definition `int tail(int (*row)[4])`, accesses `(*row)[3]`, returns 13.
- Moved `test/valid/pointers/test_pointer_to_array_file_scope_decl__0.c` from invalid (was `test/invalid/pointers/test_pointer_to_array_file_scope_decl__0.c`): Declares `int (*p)[10] = &row`, returns 0. This is valid C99 but was previously rejected.

## Final Results

All 1951 tests pass:
- Valid: 1267 (was 1262 in stage 134)
- Invalid: 259 (was 260 in stage 134)
- Integration: 88
- Print-AST: 50
- Print-tokens: 100
- Print-asm: 21
- Unit: 165

No regressions. Self-host C0→C1→C2 bootstrap completed successfully with all 1951 tests passing at every stage. No source changes were needed during bootstrap.

Self-host build versions:
- C0: ClaudeC99 version 00.02.13500000.01009 (GCC-built)
- C1: ClaudeC99 version 00.02.13500000.01010 (bootstrap from C0)
- C2: ClaudeC99 version 00.02.13500000.01011 (bootstrap from C1)

## Session Flow

1. Read stage spec and bug report (CC99-008 and CC99-009)
2. Reviewed existing parameter-declaration parsing and redeclaration logic
3. Identified adjustment points in `parse_parameter_declaration` and `parse_declarator`
4. Implemented array-to-pointer adjustment for array declarators
5. Implemented function-to-pointer-to-function adjustment for function declarators
6. Fixed `(void)` parameter-count bug in function-pointer parameter loop
7. Removed pointer-to-array rejection error and added `ParsedDeclarator` fields
8. Built pointer-to-array types in `parse_parameter_declaration`
9. Added 6 new test cases covering all four patterns
10. Ran full test suite: all 1951 tests passing
11. Ran self-host bootstrap (C0→C1→C2): all tests pass at every stage
12. Generated milestone summary, transcript, and README update
