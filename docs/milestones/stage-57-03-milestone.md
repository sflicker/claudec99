# Milestone Summary

## Stage 57-03 Variadic Function Calls and Declarations: Limited Variadic Support - Complete

stage-57-03 ships support for declaring and calling variadic functions with the `...` ellipsis syntax.

- Tokenizer: Added `TOKEN_ELLIPSIS` token type and detection of three-dot sequence `...` as a single token.
- Grammar/Parser: Function parameter lists now support optional trailing `...` after the parameter declarators; parser consumes the ellipsis and marks the function as variadic.
- AST/Semantics: Added `is_variadic` field to `ASTNode` and `FuncSig`; call-site validation allows at least N arguments (not exactly N) for variadic functions.
- Codegen: Emits `xor eax, eax` before `call` instructions for variadic functions (SysV AMD64 protocol requires AL to specify float argument count).
- Tests: Added 2 new valid tests (printf with no args, printf with one int arg); all 1028 tests pass.
- Docs: Updated `docs/grammar.md` parameter_list rule to include optional `"," "..."` syntax; updated README.md through stage 57-03 with variadic function declaration support noted, removed "variadics" from unsupported list with note that function definitions and vararg macros remain unsupported.
- Notes: Variadic function *definitions* (using `va_list`, `va_start`, `va_arg`, `va_end`, `<stdarg.h>`) are not supported; only declarations and calls to external variadic functions (like `printf`) are supported.
