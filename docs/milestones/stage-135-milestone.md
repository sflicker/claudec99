# Milestone Summary

## Stage 135: Type Compatibility and Composite Type Checks - Complete

Stage 135 fixes two C99 type compatibility bugs affecting function parameters: array-to-pointer adjustment validation (CC99-008) and pointer-to-array type support (CC99-009).

- **Parser**: `parse_parameter_declaration` now applies array-to-pointer and function-to-pointer-to-function adjustments for named declarators before compatibility checks; `parse_declarator` supports `(*name)[N]` patterns instead of rejecting them. `ParsedDeclarator` gains three fields (`is_ptr_to_array`, `ptr_to_array_length`, `ptr_to_array_has_size`).
- **Semantics**: Array parameters (`int a[3]`, `int a[]`) and pointer parameters (`int *a`) form compatible function types per C99 §6.7.5.3p7. Function-type parameters (`int cb(void)`) adjust to pointer-to-function per §6.7.5.3p8. Composite type rules apply after adjustment. Fixed pre-existing bug: `(void)` in function-pointer parameter lists was counted as one void parameter instead of zero.
- **Type system**: Pointer-to-array types (`int (*row)[4]`, `int (*row)[]`) are now valid. Indexed access (`(*row)[i]`) works via existing codegen path.
- **Tests**: 6 new tests added (5 valid, 1 moved from invalid→valid). Coverage: array-parameter compatibility variants (bounded, unbounded, pointer forms), function-parameter adjustment, pointer-to-array indexed access, and the CC99-008 and CC99-009 reduced examples. One pre-existing invalid test (`test_pointer_to_array_file_scope_decl__0`) moved to valid since pointer-to-array is valid C99.
- **Docs**: Updated README.md (stage through-line, test totals: 1951 passing) and updated version number (13500000).
- **Results**: All 1951 tests pass (1267 valid, 259 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with all tests passing at every stage, no source changes needed during bootstrap.
