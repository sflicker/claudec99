# Stage 26 General Declarator Cleanup — Kickoff

## Summary of the Spec

Stage 26 is a refactoring stage that removes the special `<func_ptr_declarator>` grammar path and replaces it with a unified recursive `<direct_declarator>` grammar. This consolidates function pointer declarator handling into the same machinery used for ordinary identifiers, pointers, arrays, and function declarators.

Key changes:
1. **Grammar update**: `<direct_declarator>` now uses recursive suffixes: `<direct_declarator> "[" ... "]"` and `<direct_declarator> "(" ... ")"` instead of identifier-specific forms
2. **New grammar rule**: `<parameter_declarator> ::= <type_specifier> [ <declarator> ]` — declarator is now optional (allows unnamed parameters)
3. **Remove grammar rules**: `<func_ptr_declarator>`, `<func_ptr_param_list>`, `<func_ptr_param>`
4. **Backward compatibility**: All existing function pointer functionality from stage 25 must continue working
5. **Parameter naming**: Unnamed params are allowed in function prototypes and function pointer signatures; definition params must still be named

## Required Tokenizer Changes

None required.

## Required Parser Changes

Changes to `src/parser.c`:

- Add `fp_param_types[FUNC_TYPE_MAX_PARAMS]` and `fp_param_count` fields to `ParsedDeclarator` struct
- In `parse_declarator`: when the function-pointer case is detected (parenthesized declarator with inner stars + trailing `(`), consume the `(params)` list immediately inline instead of leaving it for the caller
- Remove `parse_func_ptr_param_types` and `build_func_ptr_type` as separate functions
- Update three callers (`parse_statement`, `parse_parameter_declaration`, `parse_external_declaration`) to build the function-pointer Type* inline rather than calling `build_func_ptr_type`
- Make declarator optional in `parse_parameter_declaration` (allow unnamed params)
- Skip duplicate-name check for unnamed params in `parse_parameter_list`
- Add validation in `parse_external_declaration` that function definition params are all named
- Update error message for the `int (*fp())(int)` case (functions returning function pointers remain out of scope)

## Required AST Changes

None required.

## Required Code Generation Changes

None required.

## Test Plan

**New tests to add:**
- `test_func_ptr_unnamed_params__0.c`: Function pointer with unnamed params
- `test_proto_unnamed_param__0.c`: Function prototype with unnamed params
- Invalid test for unnamed param in function definition

**Test maintenance:**
- Rename `test_invalid_122__function_pointers_are_not_supported.c` to reflect the updated error message for the nested function declarator case
- Verify all existing function pointer tests from stage 25 continue to pass
- Verify all existing declarator tests continue to pass

## Implementation Approach

This refactoring consolidates function pointer declarator parsing with the general declarator machinery. The key insight is that the function-pointer declarator case (a parenthesized declarator with pointer stars followed by parameters) can be detected and handled inline within `parse_declarator`, eliminating the need for separate grammar paths. This allows parameters to be consumed and validated during declarator parsing rather than deferred to the caller.

Progress will be tracked in small implementation steps, pausing after each subsystem to verify correctness.
