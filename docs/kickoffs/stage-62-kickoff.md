# Stage 62 Kickoff

## Summary

Stage 62 adds two controlled stub headers (`limits.h` and `stdint.h`) to `test/include/` to enable testing of standard integer type and limit macros. These headers provide:

- **limits.h**: Character and integer type limits (CHAR_BIT, signed/unsigned char, short, int, long ranges)
- **stdint.h**: Fixed-width integer type definitions (int8_t through int64_t, uint8_t through uint64_t), pointer-size types (intptr_t, uintptr_t), and fast/minimum width variants

No compiler source changes are required—the compiler already supports all necessary preprocessor and typedef functionality.

## Known Spec Issues

The spec file contains several errors that will be corrected during implementation:

1. **Spec title**: Says "Stage 61" instead of "Stage 62" (copy-paste error)
2. **limits.h typo**: "SCAR_MAX" should be "SCHAR_MAX" (missing H)
3. **limits.h value error**: "SHRT_MIN (-32868)" should be "SHRT_MIN (-32768)" (digit transposition)
4. **stdint.h typo**: "typedef unsigned ln09g uint64_t;" should be "typedef unsigned long uint64_t;" (garbled)
5. **Missing code fence**: No closing ``` after stdint.h definition before tests section
6. **Incomplete test**: Second test case is missing closing brace and code fence

## Required Changes

### New Files

1. **test/include/limits.h** — Character and integer limit macros
2. **test/include/stdint.h** — Fixed-width integer type definitions
3. **test/valid/test_stdint_sizeof_types__46.c** — Test stdio types with size calculations
4. **test/valid/test_stdint_uintptr_address__1.c** — Test pointer-to-integer cast
5. **test/valid/test_limits_char_bit__8.c** — Test CHAR_BIT macro

### Tokenizer Changes

None required.

### Parser Changes

None required.

### AST Changes

None required.

### Code Generation Changes

None required.

## Test Plan

- **test_stdint_sizeof_types__46.c**: Validates all 10 integer type definitions have correct sizes; expects return value 46
- **test_stdint_uintptr_address__1.c**: Validates pointer-to-uintptr_t casts and NULL comparison work
- **test_limits_char_bit__8.c**: Validates CHAR_BIT macro is accessible and returns correct value

All tests should compile and execute successfully as valid tests.

## Implementation Order

1. Create `test/include/limits.h` with corrected definitions
2. Create `test/include/stdint.h` with corrected definitions
3. Create and validate test cases
4. Run full test suite
