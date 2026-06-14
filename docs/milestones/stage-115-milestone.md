# Milestone Summary

## Stage 115 — Constant Expressions in Array Dimension Bounds - Complete

stage-115 extends array-dimension parsing to accept full C99 integer constant expressions (arithmetic, bitwise, shift, relational, equality, logical AND/OR, ternary, sizeof, parentheses, enum constants, macro identifiers) instead of bare integer literals.

- **Parser**: Four sites in `src/parser.c` that previously read only `TOKEN_INT_LITERAL` as array bounds now call `eval_const_expr(parser, "array dimension")` — (A) `parse_type_name` bracket loop for `sizeof(int[N])` and compound literals, (B) `parse_direct_declarator` parenthesized form for `int (*a)[N]`, (C) non-paren first dimension for `int a[N]` and typedefs, (D) non-paren additional dimensions for `int a[N][M]` second+ dims.
- **Tests**: 9 new files added (7 valid cases: global/local/multidim arrays, typedef, struct member, sizeof; 2 invalid cases: runtime variable rejection, negative const-expr rejection); 3 existing invalid test files renamed to reflect new error messages when `eval_const_expr` replaced literal-only reads.
- **Codegen/AST**: No changes. Array dimensions evaluated at parse time; evaluator already implemented in stages 99–104.
- **Build**: All 1850 tests pass (1168 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit).
- **Self-host**: C0→C1→C2 passes cleanly (00.02.01150000.00906/07/08); no compiler source changes needed during bootstrap.
- **Docs**: Version bumped to `"01150000"` in `src/version.c`; self-compilation report updated.
- **Notes**: Three invalid tests renamed because error messages changed from "array size must be an integer literal" to either "is not an integer constant expression" (for runtime vars) or "array size must be greater than zero" (for negative exprs). Compiler's own source uses only literal constants in array dimensions, so bootstrap paths are unaffected.
