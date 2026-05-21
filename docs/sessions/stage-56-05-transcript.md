# stage-56-05 Transcript: Standard Predefined Macros

## Summary

This stage adds three standard predefined macros to the ClaudeC99 preprocessor: `__STDC__` (expands to 1), `__STDC_VERSION__` (expands to 199901), and `__CLAUDEC99__` (expands to 1). These macros are automatically defined at the start of preprocessing, before any user code is processed, and are available in `#if`, `#ifdef`, and normal macro expansion contexts. They behave as ordinary object-like macros, supporting compatible redefinitions and `#undef` operations, while incompatible redefinitions are rejected by the existing macro-safety checks.

The implementation required only preprocessor changes: three `macro_define` calls were inserted into the macro table initialization in `src/preprocessor.c`. No tokenizer, parser, AST, or code generator changes were needed.

## Changes Made

### Step 1: Preprocessor Initialization

- Modified `preprocess_with_defines_and_includes()` in `src/preprocessor.c` to define three predefined macros before the user-supplied `-D` definitions are processed
- Added three `macro_define()` calls:
  - `__STDC__` → 1
  - `__STDC_VERSION__` → 199901
  - `__CLAUDEC99__` → 1
- These macros are stored in the macro table with their full names and expansions; they persist throughout preprocessing and are subject to the same redefinition and `#undef` rules as user-defined macros

### Step 2: Testing

- Created `test/valid/test_pp_predef_stdc_version_if__42.c`: Tests `__STDC_VERSION__` in an `#if` conditional; expects exit code 42
- Created `test/valid/test_pp_predef_claudec99_ifdef__42.c`: Tests `__CLAUDEC99__` with `#ifdef` and normal expansion; expects exit code 42
- Created `test/valid/test_pp_predef_stdc_undef__1.c`: Tests that `#undef __STDC__` removes the macro and reverts to undefined-identifier behavior (evaluates to 0 in `#if`); expects exit code 1
- Created `test/invalid/test_pp_predef_redefine_incompatible__incompatible_replacement.c`: Tests that incompatible redefinition of `__STDC__` is rejected by the compiler (e.g., `-D__STDC__=2` conflicts with predefined `__STDC__=1`)

### Step 3: Bug Fixes During Implementation

- **Initial Implementation Bug**: Used hardcoded string length 15 for `__STDC_VERSION__` (16 characters including null terminator), which caused a one-byte overflow. Fixed by switching to `strlen()` calls for all three macro names and expansions, ensuring correct length computation.
- **Spec Typo Correction**: Specification contained `___STDC__` (three underscores) in the test code; corrected to `__STDC__` (two underscores) per the C99 standard. Also noted missing `#endif` in the spec test; this was added to the actual implementation tests.
- **Invalid Test Naming**: Ensured the invalid test file followed the naming convention with the `__incompatible_replacement` fragment to indicate the error type.

## Final Results

All 1014 tests pass (0 failed):
- valid: 628 passed (+3 new tests)
- invalid: 196 passed (+1 new test)
- integration: 31
- print_ast: 39
- print_tokens: 99
- print_asm: 21

Clean build, no regressions.

## Session Flow

1. Read stage spec to understand the three predefined macros and their requirements
2. Reviewed existing macro handling in `src/preprocessor.c` to understand the macro table and `macro_define` function signature
3. Identified correct placement for predefined macros (at the start of `preprocess_with_defines_and_includes`, before user `-D` definitions)
4. Implemented three `macro_define` calls with macro names, value expansions, and string lengths
5. Discovered and fixed initial bug: hardcoded length 15 for `__STDC_VERSION__` (actually 16 chars); switched to `strlen()`
6. Noted spec typo: `___STDC__` (3 underscores) corrected to `__STDC__` (2 underscores); added missing `#endif` to test
7. Created four test cases covering `#if` conditions, `#ifdef` conditionals, normal expansion, and incompatible redefinition rejection
8. Verified test naming conventions (`__<fragment>` suffix for invalid tests)
9. Built compiler and ran full test suite
10. Confirmed all 1014 tests pass with no regressions; recorded results
