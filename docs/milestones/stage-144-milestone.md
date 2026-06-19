# Milestone Summary

## Stage 144: Constant Unary Folding - Complete

stage-144 ships constant unary folding in the optimizer: `AST_UNARY_OP` nodes whose operand is an `AST_INT_LITERAL` are now folded for all four unary operators under `-O1`.

- Tokenizer: no changes.
- Grammar/Parser: no changes.
- AST/Semantics: no changes.
- Codegen/Optimizer: replaced stage-143 ~-only unary folding block in `optimize_expr()` with a unified rule covering `-` (arithmetic negation), `+` (identity), `!` (logical NOT), and `~` (bitwise complement); operators applied to non-constant operands are unaffected.
- Tests: 4 new integration tests (unary_minus, unary_plus, unary_not, unary_combined). Full suite 1992/1992 pass (165 unit, 1286 valid, 261 invalid, 109 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- Docs: stage spec, kickoff, milestone summary, and transcript added.
- Notes: unary fold rules inherit operand type for `-` and `+`; `!` produces `TYPE_INT` 0 or 1; `~` inherits operand type.
