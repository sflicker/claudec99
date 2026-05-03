# Milestone Summary

## Stage 17-02: sizeof Expressions - Complete

stage-17-02 ships support for `sizeof` with expression operands, extending stage 17-01's type-name syntax to handle bare variable references and arbitrary expressions.

- **Grammar/Parser**: Added `sizeof <unary_expression>` non-parenthesized form and `sizeof(<expression>)` parenthesized expression form; reworked `parse_unary` to distinguish all three cases and reject empty `sizeof()`.
- **AST**: Added `AST_SIZEOF_EXPR` node type to represent sizeof applied to an expression operand.
- **Codegen**: Implemented `sizeof_type_of_expr()` helper to resolve expression type without evaluation or integer promotion of bare variables; added handling for `AST_SIZEOF_EXPR` to emit compile-time constant size as `mov rax, <size>`.
- **Type Semantics**: Correctly applies integer promotion and usual arithmetic conversions for binary expressions (e.g., `char + char` → `int` → size 4); dereference and subscript return pointee/element type directly.
- **Tests**: Added 18 valid test files covering basic variables (char/short/int/long), both sizeof syntaxes, arithmetic promotions, pointer dereference, array subscripts, and side-effect suppression (++/=); added 3 invalid test files for bare/empty/malformed sizeof.
- **Docs**: Updated `docs/grammar.md` with sizeof expression alternative; updated `README.md` stage description and "Through stage" marker to 17-02.
- **Results**: Full test suite: 577 passed (up from 556). All 21 new tests pass. No regressions.
- **Notes**: Spec had minor issues (title typo "expresions", missing parens/semicolons in examples, code block formatting); all addressed in implementation.
