# Milestone Summary

## Stage 57-04: Variadic Macros - Complete

stage-57-04 ships support for variadic function-like macros with `__VA_ARGS__` substitution.
- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: No changes.
- Preprocessor: Added `is_variadic` field to `MacroDef` struct. Updated `#define` parsing to recognize `...` as the last parameter (both `#define M(...)` and `#define M(x, ...)` forms). Updated macro expansion to accept `arg_count >= param_count` for variadic macros, build `__VA_ARGS__` from extra arguments as a comma-separated token sequence, and perform argument substitution seamlessly.
- Tests: 2 valid tests (LOG macro and output), 1 invalid test (too few fixed args). Full suite 1030/1030 pass (638 valid, 202 invalid, 31 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- Docs: README updated through stage 57-04; no grammar changes needed.
- Notes: GNU extensions (named variadic args, `__VA_OPT__`, comma deletion) are out of scope.
