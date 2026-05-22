# stage-57-04 Transcript: Variadic Macros

## Summary

Stage 57-04 adds support for variadic function-like macros in the preprocessor. The implementation allows macros to be defined with `...` as a final parameter—either as the only parameter (`#define M(...)`) or after fixed parameters (`#define M(x, ...)`). When such a macro is invoked with more arguments than fixed parameters, the extra arguments are collected into a synthetic `__VA_ARGS__` parameter and substituted into the replacement list as a comma-separated token sequence. The implementation validates that all required fixed parameters are provided at the call site and rejects calls with insufficient arguments. No changes to the tokenizer, parser, AST, or code generator are needed; all changes are isolated to the preprocessor subsystem.

## Changes Made

### Step 1: Preprocessor Structure

- Added `int is_variadic` field to the `MacroDef` struct to track whether a macro definition accepts variable arguments.
- Updated `macro_define()` function signature to accept an `is_variadic` parameter.
- Added `is_variadic` field to the compatible-redefinition check so that redefinitions must match the variadic status of the original definition.

### Step 2: Macro Definition Parsing

- Updated `#define` directive parsing to recognize `...` as a valid final parameter in function-like macros.
- Two forms are supported:
  - `#define M(...)` — variadic with no fixed parameters.
  - `#define M(x, ...)` — variadic with one or more fixed parameters.
- The parser sets `is_variadic = 1` when `...` is detected and consumes it as the final parameter token.
- `...` in any position other than last, or multiple `...` tokens, are rejected at parse time.

### Step 3: Replacement Text Validation

- Updated stringification (`#`) validation to accept `__VA_ARGS__` as a valid parameter to stringify in variadic macros.
- Existing rules for object-like macros and non-parameter stringification remain unchanged.

### Step 4: Macro Expansion (Preprocessor Internal)

- Updated the first expansion call site (`preprocess_internal`) to handle variadic macro invocations.
- For variadic macros, the argument count check is relaxed: `arg_count >= param_count` (where `param_count` excludes the `...` token itself).
- If extra arguments are present, they are joined with `, ` (comma-space) to form the `__VA_ARGS__` string.
- An extended `params` array is built with `__VA_ARGS__` appended as a synthetic parameter.
- An extended `args` array is built with the `__VA_ARGS__` string appended as a synthetic argument.
- The extended arrays are passed to `substitute_args()` for seamless substitution.

### Step 5: Macro Expansion (Text Processing)

- Updated the second expansion call site (`expand_macros_text`) to apply the same logic for variadic macro expansion during nested preprocessing.
- Both call sites now handle variadic macros consistently.

### Step 6: Tests

- Added `test/valid/test_pp_variadic_macro_log__0.c`: Spec test defining `#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)` and calling it with a format string and one extra argument; outputs "hello".
- Added `test/valid/test_pp_variadic_macro_log__0.expected`: Expected output "hello\n".
- Added `test/invalid/test_invalid_145__expected_at_least.c`: Test case validating that calling a variadic macro with too few fixed arguments is rejected with a diagnostic error.

## Final Results

All 1030 tests pass (638 valid, 202 invalid, 31 integration, 39 print-AST, 99 print-tokens, 21 print-asm). No regressions. The new tests validate correct variadic macro expansion and error reporting for insufficient fixed arguments.

## Session Flow

1. Read spec and review stage requirements for variadic macros.
2. Examined existing macro infrastructure in `src/preprocessor.c`.
3. Identified required changes: `is_variadic` field, parsing logic, expansion logic with `__VA_ARGS__` synthesis.
4. Implemented `is_variadic` field in `MacroDef` struct and updated `macro_define()` signature.
5. Updated `#define` parsing to recognize and validate `...` as the final parameter.
6. Updated replacement text validation to allow `__VA_ARGS__` in stringification for variadic macros.
7. Enhanced both expansion call sites to build extended parameter/argument arrays and perform `__VA_ARGS__` synthesis.
8. Added test cases: valid LOG macro expansion and invalid insufficient-argument error.
9. Built and ran full test suite; verified all 1030 tests pass with no regressions.
10. Generated milestone summary and transcript artifacts.
