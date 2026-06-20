# Milestone Summary

## Stage 148: Negation Folding - Complete

stage-148 extends the stage-142 optimizer with **negation folding**: reducing `-(-x)` to `x` for non-constant expressions.

- **Optimizer**: New rule block in `src/optimize.c` within `optimize_expr`, inserted after the stage-147 double logical NOT block and before the stage-145 algebraic identity block. Detects outer `AST_UNARY_OP("-")` whose child is also `AST_UNARY_OP("-")` with a non-literal operand. Nulls inner node's child slot before `ast_free` to prevent double-free of `x`, then returns inner operand directly.
- **Double-negation folding**: `-(-x) → x` for non-constant `x`. Constant cases `-(-const)` already handled by stage-144 two-pass constant unary folding and are not re-implemented. Memory management pattern identical to stage-147 `!!x` block.
- **Composition**: Folding correctly composes with other optimizer rules in a single bottom-up pass; e.g., `-(-x) + 0 → x`.
- **Tests**: Three new integration tests (test_neg_fold_double_minus, test_neg_fold_triple_minus, test_neg_fold_combined), all using `-O1`. All existing tests pass at both `-O0` and `-O1`.
- **Results**: All 2012 portable tests pass (165 unit, 1286 valid, 261 invalid, 129 integration, 50 print-AST, 100 print-tokens, 21 print-asm). 3 new integration tests added; 126 integration tests from prior stages continue to pass.
- **Self-host**: C0→C1→C2 verified with all tests passing at every stage. No source changes needed during bootstrap.
- **Version**: `src/version.c` bumped to `"01480000"`.
