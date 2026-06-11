# Milestone Summary

## Stage 100: File-Scope Constant Expressions - Complete

stage-100 ships file-scope constant expressions, allowing integer-typed global variables to accept full compile-time constant expressions (arithmetic, bitwise, shift, and unary operators) as initializers, plus `sizeof(type-name)` support in constant-expression contexts.

- **Parser**: Extended `eval_const_primary` with `TOKEN_SIZEOF` handling; added `TOKEN_ENUM` to type-start condition for `sizeof(enum Color)` support. Updated first-declarator and multi-declarator file-scope initializer paths in `parse_external_declaration` to call `eval_const_expr` for integer-typed globals instead of requiring literals only. Pointer-typed globals retain the original literal-only validation path.
- **Tests**: Added 10 valid tests (arithmetic, bitwise-or, shift, sizeof-ptr, sizeof-expr, sizeof-struct, enum-const with operator, unary-negation, bitwise-complement, multi-declarator list), 2 invalid tests (variable reference, sizeof without parens), 1 print-ast test confirming constant-folding at parse time, and 2 renamed invalid tests (error message changed to "file-scope initializer is not an integer constant expression").
- **Version**: Bumped VERSION_STAGE to "01000000".
- **Self-host**: C0→C1→C2 all 1544/1544 tests pass. No regressions.
- **Docs**: Updated README.md with new feature description and test totals; grammar.md updated with `<file-scope-initializer>` and `<const_primary>` rules to reflect constant expressions and `sizeof(type-name)`.
- **Notes**: Only `sizeof(type-name)` is supported in constant expressions; `sizeof(expr)` remains unsupported. Block-scope static initializers are unchanged (out of scope). Pointer-typed and struct/union-typed globals follow separate validation paths and accept only literals.
