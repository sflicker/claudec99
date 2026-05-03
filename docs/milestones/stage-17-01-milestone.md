# Milestone Summary

## stage-17-01: sizeof Type Names - Complete

stage-17-01 ships `sizeof(<type>)` support, allowing queries of type sizes at compile time.

- Tokenizer: Added `TOKEN_SIZEOF` keyword; lexer recognizes `"sizeof"`.
- Grammar/Parser: Extended unary expressions to accept `"sizeof" "(" <type> ")"` form; parser consumes TOKEN_SIZEOF, expects `(`, parses base type keyword (char/short/int/long) optionally followed by `*` for pointer types, expects `)`, builds AST_SIZEOF_TYPE node.
- AST/Semantics: Added `AST_SIZEOF_TYPE` node type with `decl_type` field storing TypeKind.
- Codegen: Added `AST_SIZEOF_TYPE` handling in `expr_result_type` (returns TYPE_LONG) and `codegen_expression` (emits `mov rax, N` where N is 1, 2, 4, or 8 per type).
- Tests: 5 new tests (sizeof_char__1, sizeof_short__2, sizeof_int__4, sizeof_long__8, sizeof_int_ptr__8). Full suite 556/556 pass.
- Docs: Updated `docs/grammar.md` unary_expression rule; added sizeof feature to README.md.
- Notes: Stage limited to sizeof with explicit type names; sizeof expressions (e.g., `sizeof x`) out of scope.
