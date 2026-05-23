# stage-60-01 Transcript: Static Predefined Target Macros

## Summary

Stage 60-01 adds 12 static predefined macros to the preprocessor, injected unconditionally before user source preprocessing begins. These macros reflect the x86_64 Linux LP64 ABI and include platform identifiers (`__x86_64__`, `__linux__`, `__unix__`, `__LP64__`, `_LP64`) and size constants (`__CHAR_BIT__`, `__SIZEOF_CHAR__`, `__SIZEOF_SHORT__`, `__SIZEOF_INT__`, `__SIZEOF_LONG__`, `__SIZEOF_POINTER__`, `__SIZEOF_SIZE_T__`). All macros are defined directly in the preprocessor code via `macro_define()` calls in the `preprocess_file()` function, not loaded from an external header file. These macros behave like ordinary object-like macros and are available in `#if`, `#ifdef`, and normal source macro expansion.

## Changes Made

### Step 1: Preprocessor

- Added 12 `macro_define()` calls in `src/preprocessor.c` within the `preprocess_file()` function, immediately after defining `__STDC__`, `__STDC_VERSION__`, and `__CLAUDEC99__`.
- Platform macros defined: `__x86_64__` = 1, `__linux__` = 1, `__unix__` = 1, `__LP64__` = 1, `_LP64` = 1.
- Size/ABI macros defined: `__CHAR_BIT__` = 8, `__SIZEOF_CHAR__` = 1, `__SIZEOF_SHORT__` = 2, `__SIZEOF_INT__` = 4, `__SIZEOF_LONG__` = 8, `__SIZEOF_POINTER__` = 8, `__SIZEOF_SIZE_T__` = 8.
- All macros are unconditional and always available before source preprocessing.

### Step 2: Tests

- Added 6 new valid test cases to `test/valid/`:
  - `test_pp_predef_x86_64_ifdef__42.c`: Tests `__x86_64__` availability in `#ifdef`.
  - `test_pp_predef_linux_ifdef__42.c`: Tests `__linux__` availability in `#ifdef`.
  - `test_pp_predef_unix_ifdef__42.c`: Tests `__unix__` availability in `#ifdef`.
  - `test_pp_predef_lp64_if__42.c`: Tests `__LP64__` and `_LP64` in `#if` conditions.
  - `test_pp_predef_sizeof_types_if__42.c`: Tests `__SIZEOF_*` macros in `#if` conditions.
  - `test_pp_predef_char_bit_if__42.c`: Tests `__CHAR_BIT__` in `#if` conditions.

## Final Results

Build: Clean, no errors.

Test results: All 1058 tests pass (644 valid, 202 invalid, 53 integration, 39 print-AST, 99 print-tokens, 21 print-asm). Stage 59 baseline was 1052 tests; stage 60-01 adds 6 new valid tests. No regressions.

## Session Flow

1. Read the stage spec and template requirements.
2. Examined the preprocessor implementation in `src/preprocessor.c`.
3. Identified the injection point: immediately after existing predefined macros in `preprocess_file()`.
4. Added 12 `macro_define()` calls for platform and size macros.
5. Created 6 test cases covering ifdef/if conditions and macro values.
6. Ran the full test suite: `./test/run_all_tests.sh`.
7. Verified all 1058 tests pass with no regressions.
8. Generated milestone, transcript, and README updates.
