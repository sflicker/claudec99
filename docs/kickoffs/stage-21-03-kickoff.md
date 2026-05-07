# Stage 21-03 Kickoff: Function Declaration Compatibility Checks

## Summary

Stage 21-03 adds comprehensive compatibility checks between function declarations and their later definitions. The compiler will verify that declarations and definitions match on return type and parameter types, in addition to existing checks for name, parameter count, single definition, and initializer validity.

**What's already checked:**
- Same function name
- Same parameter count
- Only one function definition
- Function declarations cannot have initializers

**What's new in this stage:**
- Return type compatibility between declaration and definition
- Parameter type compatibility between declaration and definition

**Spec note:** The valid test "Repeating matching prototype" contains a typo where the closing brace is written `{` instead of `}`. Implementation follows the obvious intent.

## Required Tokenizer Changes

None. All necessary tokens are already available.

## Required Parser Changes

1. **Enhance `parser_register_function()` in `src/parser.c`**
   - Add return type mismatch check when a function is already declared
   - Add parameter type mismatch check by iterating through declared and current parameter lists
   - Emit diagnostic errors for type mismatches (distinct from existing parameter count mismatch)

## Required AST Changes

None. The existing function declaration/definition structure supports storing type information needed for compatibility checking.

## Required Code Generation Changes

None. Code generation is unaffected; only validation is enhanced.

## Test Plan

**Valid cases (passing tests):**
- `test_multiple_decls__42.c` — multiple matching declarations before definition (existing)
- `test_decl_forward_call__42.c` — forward declaration before use (existing)
- `test_proto_identity_ptr__7.c` — pointer return and pointer parameter (existing)

**Invalid cases requiring new tests:**

1. **Return type mismatch** — `test_invalid_107__return_type_mismatch.c`
   - Declaration: `int f(int a);`
   - Definition: `long f(int a) { ... }`
   - Expected: diagnostic error, compilation fails

2. **Parameter type mismatch** — `test_invalid_108__parameter_type_mismatch.c`
   - Declaration: `int f(char a);`
   - Definition: `int f(int a) { ... }`
   - Expected: diagnostic error, compilation fails

**Invalid cases already covered:**
- Duplicate definition — `test_invalid_5__duplicate_function.c` (existing)
- Parameter count mismatch — `test_invalid_10__parameter_count_mismatch.c` (existing)
- Function initializer — `test_invalid_106__function_declaration_cannot_have_an_initializer.c` (existing)

## Implementation Order

1. Enhance parser validation in `src/parser.c`
2. Add `test_invalid_107__return_type_mismatch.c`
3. Add `test_invalid_108__parameter_type_mismatch.c`
4. Run all tests to verify no regressions
