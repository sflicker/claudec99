# stage 25-02 Kickoff: Assign Function Designators to Function Pointers

## Spec Summary

Allow function names used as values (function designators) to initialize or assign compatible function pointers. Reject assignments if the function type is incompatible.

Two main test scenarios:
- Local variable assignment: `fp = inc;` where `fp` is a local function pointer
- File-scope initialization: `int (*fp)(int) = inc;` at file scope
- Incompatible type rejection: different return types or parameter types

## Tokenizer Changes

None required.

## Parser Changes

File-scope function pointer declaration branch must:
- Allow `= identifier` initializer syntax
- Validate type compatibility between function designator and pointer target type
- Reject if types are incompatible

## AST Changes

No AST node type changes required. Existing structures sufficient.

## Code Generation Changes

### Helper Functions
- `func_ptr_types_equal_cg`: Check function pointer type compatibility
- `type_from_kind`: Extract type information from kind
- `build_func_designator_type`: Build type descriptor for a function designator

### Handler Updates
- `AST_VAR_REF`: Recognize function designators and emit `lea rax, [rel name]`
- `AST_ASSIGNMENT` (locals and globals): Type-check when LHS is function pointer
- `codegen_add_global`: Support label-reference initializers
- `codegen_emit_data`: Support label-reference initializers

### Data Structure Updates
- `GlobalVar` struct: Add `init_label[256]` and `is_label_init` fields

## Test Plan

### Remove
- `test_invalid_127` (becomes valid under this stage)

### Add Valid Tests
- `test_func_ptr_assign_local__0.c`: Local function pointer assignment
- `test_func_ptr_assign_file_scope__0.c`: File-scope function pointer initialization

### Add Invalid Tests
- `test_invalid_128__incompatible_function_pointer_type.c`: Return type mismatch
- `test_invalid_129__incompatible_function_pointer_type.c`: Parameter type mismatch

## Implementation Order

1. Add GlobalVar struct fields and helpers for label-based initialization
2. Implement `func_ptr_types_equal_cg`, `type_from_kind`, `build_func_designator_type`
3. Update `AST_VAR_REF` codegen to handle function designators
4. Update `AST_ASSIGNMENT` codegen for function pointer type checking
5. Update `codegen_add_global` and `codegen_emit_data` for initializers
6. Update parser to allow and validate initializers in file-scope declarations
7. Implement and test

## Known Issues / Notes

Spec has minor typo: "be be" in goal statement (no ambiguity, duplicate word).
