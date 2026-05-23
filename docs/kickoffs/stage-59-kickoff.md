# Stage 59 Kickoff

## Spec Summary

Stage 59 adds 10-20 integration tests exercising the preprocessor and mock standard headers from stage 58, combined with other core compiler features. No compiler source changes are required—this is purely a test addition stage.

**Key activities:**
- Add integration tests covering preprocessor functionality (#if, #undef, stringification, token pasting, __VA_ARGS__)
- Exercise mock standard headers (stdio.h, stddef.h, stdlib.h, string.h) with real compiler features
- Collectively demonstrate major features of the supported C subset

## Derived Stage Values

**Planned test count:** 15 integration tests

**Test list:**
1. `test_pp_if_arithmetic` — #if with arithmetic + printf
2. `test_pp_undef` — #undef/redefine + return value
3. `test_pp_stringify` — stringification macro + puts
4. `test_pp_token_paste` — ## token paste + variable
5. `test_pp_va_args_macro` — __VA_ARGS__ + printf
6. `test_pp_multi_header` — stdio.h + string.h together
7. `test_pp_struct_printf` — macros + struct + printf
8. `test_pp_enum_bitflags` — enum + bitwise ops + printf
9. `test_pp_function_ptr` — #if + function pointers + printf
10. `test_pp_while_strlen` — while loop + string.h
11. `test_pp_do_while_stdio` — #define + do/while + printf
12. `test_pp_for_array_sum` — #define N + for + arrays + += + printf
13. `test_pp_switch_macro` — #define + switch/case + puts
14. `test_pp_sizeof_types` — sizeof types + printf
15. `test_pp_typedef_malloc` — typedef struct + malloc/free + printf

**Expected test count impact:**
- Baseline: 1037 tests (638 valid, 202 invalid, 38 integration, 39 print-AST, 99 print-tokens, 21 print-asm)
- After stage: 1052 tests (integration count rises from 38 to 53)

## Required Changes

### Tokenizer Changes
None.

### Parser Changes
None.

### AST Changes
None.

### Code Generation Changes
None.

## Test Plan

**Test infrastructure used:**
- Default integration test runner with DEFAULT_IFLAGS = `-I${PROJECT_DIR}/test/include`
- No .cflags files needed—all tests use the default include path
- Mock headers already exist in stage 58: stdio.h, stddef.h, stdlib.h, string.h

**Test matrix coverage:**
- Preprocessor: #if, #undef, stringification, token pasting, __VA_ARGS__
- Control flow: while, do/while, for, switch/case
- Data types: struct, enum, typedef, function pointers
- Operations: arithmetic, bitwise, array indexing, function calls
- Standard library: printf, puts, strlen, malloc, free
- Compound features: sizeof, combined macros with control flow

## Ambiguities and Decisions

None identified. Stage 59 is purely additive with no compiler changes required. Tests are self-contained and do not depend on new language features.

## Implementation Order

1. Create test files for tests 1-5 (preprocessor fundamentals)
2. Create test files for tests 6-10 (mixed headers and control flow)
3. Create test files for tests 11-15 (compound features)
4. Run integration test suite to verify all tests pass
5. Generate milestone summary
