# Stage 65 Kickoff: Integer Conversion and Arithmetic Hardening

## Spec Summary

Stage 65 is a test-only stage that validates integer conversion and arithmetic function correctly with the new `_Bool` and `long long` types alongside the updates to `signed` and `unsigned` support introduced in previous stages.

This stage adds approximately 17 new tests to `test/valid/` covering:

- **Integer promotion**: _Bool, signed/unsigned char, and signed/unsigned short promotion behavior
- **Usual Arithmetic Conversion (UAC)**: Signed vs unsigned int/long/long long comparisons, including LP64-specific behavior
- **Assignment conversion**: Truncation of larger types to smaller unsigned types, normalization to _Bool
- **Conditional expressions**: Type unification across mixed integer types
- **Compound assignment**: Truncation and wraparound behavior
- **Arithmetic with long long**: Dominance rules and unsigned long long multiplication

The tests use project-controlled stubs from `test/include/` (stdint.h, stdbool.h).

## Required Tokenizer Changes

None. Tokenizer already supports all required tokens and suffixes from Stage 64 (long long) and Stage 63 (_Bool).

## Required Parser Changes

None. Parser already handles all required type specifiers and declarations.

## Required AST Changes

None. Type system already includes all necessary types (signed/unsigned char/short/int/long/long long, _Bool).

## Required Code Generation Changes

None. Code generation already implements:
- Integer promotion for char and short types
- Usual arithmetic conversion (UAC) rules for mixed signedness comparisons
- Assignment conversion with truncation
- _Bool value normalization on assignment
- Arithmetic operations across all integer types

## Test Plan

### Test Cases to Add to test/valid/

1. **Integer Promotion**
   - `test_bool_promotes_to_int_stdbool__1.c`: _Bool in arithmetic promotes to int
   - `test_int8_promotes_sign__1.c`: signed char preserves sign when promoted
   - `test_uint8_promotes_to_int__1.c`: unsigned char promotes to int (not necessarily unsigned int)
   - `test_int16_promotes_sign__1.c`: signed short preserves sign when promoted
   - `test_uint16_promotes_to_int__1.c`: unsigned short promotes to int

2. **Signed/Unsigned Comparisons (Usual Arithmetic Conversion)**
   - `test_uac_signed_unsigned_int_cmp__0.c`: Signed vs unsigned int comparison (-1 < 1U is false)
   - `test_uac_signed_unsigned_plain_cmp__0.c`: "signed" vs "unsigned" int comparison (same as above)
   - `test_uac_neg_literal_vs_unsigned__0.c`: Negative int literal vs unsigned int literal comparison

3. **Long Type Dominance**
   - `test_uac_long_vs_uint_lp64__1.c`: On LP64, long can represent all unsigned int values, so uint converts to long (not vice versa)
   - `test_uac_long_vs_ulong__0.c`: When comparing signed long vs unsigned long, signed converts to unsigned
   - `test_uac_long_long_dominates_int__1.c`: long long dominates int (int converts to long long)
   - `test_uac_ull_dominates_signed__0.c`: unsigned long long dominates all signed types (signed converts to ull)

4. **Assignment Conversion**
   - `test_uint8_assign_truncate__1.c`: Assignment to smaller unsigned type truncates (257U → uint8_t → 1)
   - `test_bool_assign_conversion__1.c`: Assignment to _Bool normalizes to 0 or 1

5. **Conditional Expression Type Unification**
   - `test_cond_mixed_int_types__1.c`: Mixed integer types in ternary operator unify to common type

6. **Compound Assignment**
   - `test_uint8_compound_assign_wrap__1.c`: Compound assignment truncates result (uint8_t x; x = 250; x += 10; // x == 4)

7. **Arithmetic with long long**
   - `test_uac_ull_mul_signed__1.c`: unsigned long long multiplication with signed int (UAC applies before multiplication)

### Spec Issues Found (to be corrected in test files)

1. **Missing closing braces**: Four test cases lack closing `}` in their main functions:
   - "signed long vs unsigned int on LP64"
   - "signed long vs unsigned long"
   - "unsigned long long dominates signed types"
   - "assignment conversion to smaller unsigned type"

2. **Expression errors**:
   - "long long dominates int" test uses `return x + y = 42LL;` (assignment instead of comparison; should be `x + y == 42LL`)
   - "compound assignment conversion" test uses `return x = 4U;` (assignment instead of comparison; should be `x == 4U`)

3. **Logic error**:
   - "_Bool assignment conversion" test has incorrect logic. The code assigns `x + y` (which is 0) to `b`, then returns `b == false` (expecting 1). This is backwards—the test should check `b != false` before the second assignment to correctly validate the normalization behavior.

All identified issues will be corrected during implementation.

## Implementation Order

1. Create test files in `test/valid/` with all 17 test cases
2. Verify all tests pass with existing compiler implementation
3. No source code changes required (test-only stage)

## Decisions

- Test files use project-controlled headers from `test/include/stdbool.h` and `test/include/stdint.h` to ensure portable type definitions
- Test naming follows pattern: `test_<concept>_<detail>_<variant>.c`
- All spec errors and typos are silently corrected during test creation
