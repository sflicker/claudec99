# stage-53 Transcript: Predefined Macros

## Summary

Stage 53 implements support for the C99 standard predefined macros `__FILE__` and `__LINE__`. The `__FILE__` macro expands to a string literal containing the path of the current source file (with double-quote and backslash characters properly escaped). The `__LINE__` macro expands to an integer literal representing the current line number in the preprocessed source. Both macros are expanded at the point of use during preprocessing, before the ordinary macro-table lookup. Line tracking is maintained across newlines throughout preprocessing. The feature supports both ordinary source files and nested include files, with line numbers correctly reflecting the source location within each file.

## Changes Made

### Step 1: Preprocessor

- Added `int current_line = 1;` local variable in `preprocess_internal` after existing declarations.
- Added line-count increment (`current_line++;`) in the newline handler, before the existing `line_has_content = 0;` statement.
- Refactored identifier expansion in the macro-expansion section to check for `__FILE__` and `__LINE__` by exact name length and string comparison before invoking the macro-table lookup.
- For `__FILE__` (8 characters): emit the source file path as a double-quoted string literal with `"` and `\` characters escaped as `\"` and `\\`.
- For `__LINE__` (8 characters): emit the current line number as a decimal integer literal using `snprintf`.
- All existing macro-table logic (user-defined macros and previously defined builtin-like macros) moved into an `else` branch to preserve fallback behavior.

### Step 2: Tests

- Added `test/valid/test_predefined_macro_line__0.c`: unit test that checks `__LINE__` expansion (expands to 2 on line 2; computed as `2 - 2 = 0` for exit code).
- Added `test/valid/test_predefined_macro_file__0.c`: unit test that checks `__FILE__` expansion (emits as a string literal; tested via `strlen()` and returns 0).
- Added `test/integration/test_pp_predefined_macros/`: multi-file integration test with `helper.h`, `helper.c`, and `main.c`. The test exercises `__FILE__` to identify the current source file and `__LINE__` for line-number arithmetic within an included file; expects exit code 0 on successful file-name and line-number validation.

## Final Results

All 943 tests pass: 564 valid, 193 invalid, 27 integration, 39 print-AST, 99 print-tokens, 21 print-asm. No regressions; previous test count was 940 (562 valid, 193 invalid, 26 integration, 39 print-AST, 99 print-tokens, 21 print-asm).

## Session Flow

1. Read stage 53 spec and reviewed requirements for `__FILE__` and `__LINE__`.
2. Examined preprocessor implementation and identifier expansion logic in `src/preprocessor.c`.
3. Designed line-tracking mechanism and predefined-macro check order.
4. Implemented line-count increment on newline and added `__FILE__`/`__LINE__` checks before macro-table lookup.
5. Added proper escape handling for `__FILE__` string emission.
6. Created unit tests for both macros and one integration test for cross-file scenarios.
7. Built compiler and ran full test suite; verified all 943 tests pass.
8. Generated milestone summary and transcript artifacts.

## Notes

- Predefined macros are recognized by exact name (8-character length and string match) as special identifiers during preprocessing, ensuring they expand correctly even if user code defines macros with similar names (the predefined check runs first).
- Line numbers are tracked locally and reset to 1 at the start of each preprocessed file, reflecting the semantics of the C standard where `__LINE__` reports the line number in the current translation unit.
- The string literal emitted for `__FILE__` uses proper escaping (`\"` for quotes, `\\` for backslashes) to ensure the result is a valid C string literal that the lexer can parse correctly when re-scanned.
- No changes to tokenizer, parser, AST, or code generator were required; the feature is purely a preprocessor extension.
