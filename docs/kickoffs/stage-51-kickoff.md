# Stage 51 Kickoff — Function-like Macros

## Spec Summary

Stage 51 extends the preprocessor to support function-like macros in addition to the existing object-like macro support. Function-like macros are defined with syntax `#define NAME(arg1, arg2) replacement`, where there is no whitespace between the macro name and the opening parenthesis. A space before the `(` keeps the macro as object-like (e.g., `#define F (x)` treats `(x)` as the replacement text).

Key requirements:
- Parse function-like macro definitions with parameter lists
- Expand macro invocations by collecting arguments and substituting them into the replacement text
- Support arguments as arbitrary token sequences with nesting (commas split only at depth 0)
- Support zero-argument macros like `#define VALUE() 42`
- Enforce exact argument count matching
- Preserve all existing object-like macro behavior from Stage 50

## Required Tokenizer Changes

None. The tokenizer already produces the necessary tokens for preprocessor parsing.

## Required Parser Changes

None. The parser operates on preprocessor-expanded tokens and does not process macro definitions directly.

## Required AST Changes

None. The AST structure is not affected by preprocessor macro expansion.

## Required Code Generation Changes

None. Code generation receives already-expanded tokens and does not need to handle macro details.

## Test Plan

### Valid Cases

1. **ADD(20, 22)** — Basic two-argument macro expansion
   - File: `test/valid/test_pp_fn_macro_add__42.c`
   - Expected exit code: 42

2. **SQUARE(8)** — Single-argument macro expansion
   - File: `test/valid/test_pp_fn_macro_square__64.c`
   - Expected exit code: 64

3. **ANSWER()** — Zero-argument function-like macro
   - File: `test/valid/test_pp_fn_macro_zero_arg__42.c`
   - Expected exit code: 42

4. **ADD(1+2, 3)** — Macro arguments containing operators
   - File: `test/valid/test_pp_fn_macro_expr_arg__6.c`
   - Expected exit code: 6

5. **ADD(ADD(1, 2), 3)** — Nested macro invocations
   - File: `test/valid/test_pp_fn_macro_nested__6.c`
   - Expected exit code: 6

### Invalid Cases

1. **Wrong argument count** — Mismatch between definition and invocation
   - File: `test/invalid/test_invalid_139__wrong_arg_count_for_function_like_macro.c`
   - Expected: Compilation error due to argument count mismatch

## Implementation Strategy

1. Extend `MacroDef` struct in `src/preprocessor.c` to track whether a macro is object-like or function-like and store parameter names
2. Update `#define` parsing logic to detect function-like syntax (no space between name and `(`)
3. Implement argument collection logic that respects parenthesis nesting
4. Implement argument substitution in the replacement token stream
5. Add error handling for argument count mismatches
6. Test object-like macros remain unaffected
