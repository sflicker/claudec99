# stage-51 Transcript: Function-like Macros

## Summary

Stage 51 adds function-like `#define` macro support to the preprocessor. Function-like macros extend object-like macro functionality with parameterized definitions: `#define NAME(arg1, arg2) replacement`. The implementation distinguishes function-like from object-like macros by checking for an open parenthesis immediately after the macro name (no intervening whitespace). Arguments are collected by scanning comma-separated tokens at parenthesis depth zero, pre-expanded before substitution, and the expanded results are rescanned for nested macros. The preprocessor validates exact argument-count matching and rejects invocations with mismatched counts. This stage preserves all stage 50 object-like macro functionality and passes a full test suite with no regressions.

## Changes Made

### Step 1: Preprocessor Data Structures

- Extended `MacroDef` struct with `char **params` (parameter name array) and `int param_count` field.
- Assigned param_count semantics: `-1` for object-like macros, `>=0` for function-like macros (count of parameters).
- Updated `macro_define()` function signature to accept optional params ownership; params are `NULL` for object-like macros.
- Modified `macro_table_free()` to deallocate params arrays for each macro definition.

### Step 2: Argument Collection

- Implemented `collect_args()` function: scans opening parenthesis, reads comma-separated tokens, splits only at depth-0 commas, handles nested parentheses and string/char literals verbatim, returns heap-allocated array of arg strings.
- Handles zero-argument invocations correctly: `NAME()` collects an empty list.
- Validates parenthesis nesting and properly skips over quoted strings and character literals during argument collection.

### Step 3: Argument Substitution

- Implemented `substitute_args()` function: iterates over replacement-text tokens and replaces any occurrence of a parameter name with the corresponding argument text.
- Maintains token boundaries: parameter names are replaced only when they form complete tokens, not as substrings.

### Step 4: Macro Expansion in Text

- Implemented `expand_macros_text()` function: recursively expands macro invocations within a plain text string.
- Pre-expands arguments (for function-like macros) before substitution, ensuring nested macro invocations in arguments are resolved.
- Rescans the substituted result for additional macro invocations to handle nested expansions.
- Handles both object-like and function-like macros uniformly.

### Step 5: Definition Parsing

- Extended `#define` parsing to detect function-like macro definition: after reading macro name, checks if the immediate next character is `(`.
- If `(` is present (no whitespace), parses a parameter list: zero or more comma-separated identifiers, terminated by `)`.
- If `(` is absent or preceded by whitespace, continues as object-like macro definition.
- Allocates params array and passes to `macro_define()`.

### Step 6: Invocation Expansion

- Extended identifier expansion in the main token loop: for function-like macros, peeks ahead for `(`.
- If `(` is found, calls `collect_args()`, verifies argument count matches param_count, pre-expands args via `expand_macros_text()`, calls `substitute_args()`, and rescans the result.
- Rejects invocation with diagnostic error if argument count mismatches.
- Removed old "Reject function-like macros" error block.

### Step 7: Tests

- Added 5 valid tests: `test_pp_fn_macro_add__42.c` (basic `ADD(20,22)`), `test_pp_fn_macro_square__64.c` (basic `SQUARE(8)`, spec typo corrected), `test_pp_fn_macro_zero_arg__42.c` (zero-argument `ANSWER()`), `test_pp_fn_macro_expr_arg__6.c` (expression argument `ADD(1+2,3)`), `test_pp_fn_macro_nested__6.c` (nested invocation `ADD(ADD(1,2),3)`).
- Added 1 invalid test: `test_invalid_139__argument_count_mismatch_for_macro.c` (rejects `ADD(1,2,3,4)` with diagnostic error).

## Final Results

All 913 tests pass (547 valid, 182 invalid, 25 integration, 39 print-AST, 99 print-tokens, 21 print-asm). Build succeeds with no errors or warnings. No regressions from stage 50. Stage 50 had 907 tests total; stage 51 adds 6 new tests.

## Session Flow

1. Read spec and supporting documentation
2. Reviewed macro table and preprocessing infrastructure
3. Designed data structures for function-like macros (params array, param_count field)
4. Implemented argument collection with depth-aware comma splitting
5. Implemented argument substitution and macro expansion in text
6. Extended #define parsing to detect and parse parameter lists
7. Extended invocation expansion to handle function-like macros
8. Added comprehensive test coverage (5 valid, 1 invalid)
9. Built and ran full test suite; confirmed 913 tests pass with no regressions
10. Generated milestone summary and transcript artifacts
