# Stage 52-02 Kickoff: Nested Conditional Preprocessing

## Summary of the Spec

Stage 52-02 extends conditional preprocessing to verify and test that `#ifdef`, `#ifndef`, `#else`, and `#endif` work correctly when nested. The stack-based CondFrame mechanism added in stage 52-01 already handles nesting correctly — this stage is primarily about adding comprehensive test cases for nested conditionals and an include guard pattern test.

The spec defines three valid nested test scenarios:
1. Both outer and inner macros defined (true/true) — inner true branch executes
2. Only outer macro defined (true/false) — inner false → else branch executes
3. Only inner macro defined (false/true) — outer false → outer else executes

Additionally, an include guard pattern test verifies that multiple includes of the same header are handled correctly.

Two invalid test cases check for error detection:
1. Missing #endif — outer conditional missing final #endif
2. Duplicate #else — inner #ifdef has two #else directives

## Required Tokenizer Changes

None. The tokenizer already handles all preprocessing directives required.

## Required Parser Changes

None. The parser already processes all conditional directives.

## Required AST Changes

None. AST structures are unchanged.

## Required Code Generation Changes

None. Code generation is unchanged.

## Test Plan

### Valid Tests (test/valid/)

1. **test_pp_nested_ifdef_true_true__42.c**
   - Both OUTER and INNER defined
   - Inner true branch executes, returns 42

2. **test_pp_nested_ifdef_true_false__1.c**
   - Only OUTER defined
   - Inner false → else branch executes, returns 1

3. **test_pp_nested_ifdef_false_true__2.c**
   - Only INNER defined
   - Outer false → outer else executes, returns 2

### Invalid Tests (test/invalid/)

4. **test_pp_nested_missing_endif__missing_endif.c**
   - Two `#ifdef` directives, only inner `#endif` present
   - Outer missing `#endif` → error

5. **test_pp_nested_duplicate_else__duplicate_else.c**
   - Inner `#ifdef` has two `#else` directives
   - Duplicate else error

### Integration Tests (test/integration/)

6. **test_pp_include_guard**
   - Include guard pattern: helper.h included twice
   - Guard prevents duplicate macro definition
   - Both main.c and helper.h test files required
   - Return value 42 expected

## Implementation Order

1. Add three valid nested test files
2. Add two invalid nested test files
3. Add include guard integration test directory and files
4. Run full test suite to confirm all pass
