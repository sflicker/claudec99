# Stage 53 Kickoff

## Spec Summary

Add support for standard predefined macros `__FILE__` and `__LINE__` in the preprocessor. These macros expand to the current source file path and line number respectively.

Key rules:
- `__FILE__` expands to a string literal containing the path of the current source file
- `__LINE__` expands to an integer literal containing the current source line number
- Both macros should reflect the preprocessed source location as closely as possible
- Expansion occurs in ordinary source during preprocessing
- Values should be correct in main source files and reasonable in include files

Out-of-scope: `__DATE__`, `__TIME__`, `__STDC__`, `__STDC_VERSION__`, `#line` directive, and perfect diagnostic/source-map support.

## Derived Stage Values

| Component | Change Required |
|-----------|-----------------|
| Tokenizer | None |
| Parser | None |
| AST | None |
| Code generation | None |
| Preprocessor | Yes — track line numbers, handle `__FILE__` and `__LINE__` expansion |

## Planned Changes

### Preprocessor (`src/preprocessor.c`)
- Add `current_line` counter to `preprocess_internal()` to track the current source line number
- Increment `current_line` each time a newline is consumed during preprocessing
- Add special handling for `__FILE__` and `__LINE__` identifiers:
  - Before macro table lookup, check if the identifier is `__FILE__` or `__LINE__`
  - For `__FILE__`: create a string literal token containing the current file path
  - For `__LINE__`: create an integer literal token containing the current line number
  - These expansions take precedence over any macro definitions with these names

## Test Plan

**Valid tests (2):**
1. `test/valid/test_predefined_macro_line__0.c` — `__LINE__` expansion as exit code; verify that `__LINE__` returns an integer literal that can be used as an exit code
2. `test/valid/test_predefined_macro_file__0.c` — `__FILE__` expansion in main source; verify that `__FILE__` expands to a string literal and compiles/runs

**Integration test (1):**
1. `test/integration/test_pp_predefined_macros/` — Multi-file test exercising both `__FILE__` and `__LINE__` in include context:
   - `helper.h` — header with macro definitions
   - `helper.c` — implementation file using `__FILE__` to verify correct path in included source
   - `main.c` — main file including helper, exercises `__LINE__` in both main and included context

## Implementation Order

1. Preprocessor changes in `src/preprocessor.c` to add line tracking and `__FILE__`/`__LINE__` expansion
2. Valid tests (2)
3. Integration test (1)
4. Update `docs/grammar.md` to document `__FILE__` and `__LINE__` syntax
5. Update `README.md` to mention Stage 53

## Ambiguities and Decisions

- The spec test example contains a `strchr` misuse: `strchr()` expects `(const char *, int)` but is called with `(char *, char *)`. The actual test will use correct C semantics.
- The spec test file has a minor formatting issue with the delimiter (`---- main .c test file ____` should be `---- main.c ----`). The actual test files will use proper C syntax.
- Spelling note: spec says "prefect dianostic" which should read "perfect diagnostic" — out-of-scope item.
- Line tracking is managed at the preprocessor level during token consumption, ensuring that expansions of `__LINE__` reflect the actual line number in the source at the point of expansion.
