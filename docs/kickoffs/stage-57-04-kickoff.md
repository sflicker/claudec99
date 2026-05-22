# Stage 57-04 Kickoff: Variadic Macros

## Summary

Stage 57-04 adds support for variadic macros to the preprocessor. Function-like macros can now be defined with `...` as the final parameter (`#define M(...)` or `#define M(x, ...)`), and `__VA_ARGS__` in the replacement list expands to all extra arguments passed at call sites, joined by commas.

Example:
```c
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)
LOG("%s\n", "hello")  // expands to: printf("%s\n", "hello")
```

## Required Tokenizer Changes

None. Variadic macros are a preprocessor-only feature; the tokenizer is not affected.

## Required Parser Changes

None. Variadic macros are handled entirely by the preprocessor and do not affect the parser.

## Required AST Changes

None. Variadic macros do not introduce new AST node types.

## Required Code Generation Changes

None. Code generation is unaffected by variadic macro support.

## Preprocessor Changes

1. **MacroDef struct**: Add `is_variadic` boolean field to track whether a function-like macro accepts variadic arguments.

2. **Macro definition parsing**: Update `macro_define` to:
   - Recognize `...` as the final parameter in function-like macro parameter lists
   - Store `is_variadic = true` when `...` is present
   - Reject `...` in any position other than the end of the parameter list

3. **Replacement text validation**: Allow `__VA_ARGS__` in the replacement list only for variadic macros. For non-variadic macros, `__VA_ARGS__` should be treated as an undefined macro name.

4. **Macro expansion**: Update both expansion call sites (`preprocess_internal` and `expand_macros_text`) to:
   - Accept a variable number of arguments when expanding variadic macros
   - Validate that all required fixed parameters are provided
   - Allow zero or more variadic arguments
   - Substitute `__VA_ARGS__` with the comma-separated token sequence of extra arguments

## Test Plan

**Valid test case**: `test/valid/test_pp_variadic_macro_log__0.c`
- Define `LOG(fmt, ...)` macro that wraps `printf`
- Call `LOG` with format string and variadic arguments
- Verify expansion produces correct output

**Invalid test case**: `test/invalid/` test for insufficient fixed arguments
- Define a variadic macro with required fixed parameters
- Call the macro with fewer than the required fixed arguments
- Verify compiler rejects the call

## Implementation Strategy

1. Add `is_variadic` field to `MacroDef` struct and initialize to `false`
2. Update `macro_define` parsing to recognize and store `is_variadic`
3. Update replacement text validation to allow `__VA_ARGS__` for variadic macros
4. Update expansion logic to handle variable argument counts and substitute `__VA_ARGS__`
5. Test with the provided spec example and additional edge cases
