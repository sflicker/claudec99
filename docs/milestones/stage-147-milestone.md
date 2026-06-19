# Milestone Summary

## Stage 147: Boolean and Logical Simplification - Complete

stage-147 adds boolean and logical simplification rules to the stage-142 optimizer, gated at `-O1`.

- **Optimizer**: Two new rule blocks in `src/optimize.c` within `optimize_expr`. Block 1 (after stage-144 unary folding, before stage-145 algebraic): detects `!!x` (outer NOT of inner NOT with non-literal operand) and rewrites to `x != 0` via a new `AST_BINARY_OP` node. Block 2 (after stage-146 strength reduction, before return): detects binary boolean/logical simplifications: `x && 0 → 0`, `x || nonzero → 1`, `x && nonzero → (x != 0)`, `x || 0 → (x != 0)`. Left-operand-literal cases (`0 && x`, `nonzero || x`) already handled by stage-143 short-circuit folding, not re-implemented. All rules mutate or replace nodes in place with careful memory management.
- **Double-NOT transformation**: `!!x` recognized only when inner NOT child is non-literal (constant case `!!const` already handled by stage-144). Nulls inner node's child slot before `ast_free` to prevent double-free.
- **Binary boolean simplification**: `&&` and `||` operators on constants (or variables vs. constants) folded at compile time. Rules fire when right operand is a constant and operator semantics allow simplification.
- **Tests**: Six new integration tests (test_bool_simplify_and_zero, test_bool_simplify_or_nonzero, test_bool_simplify_and_one, test_bool_simplify_or_zero, test_bool_double_not, test_bool_simplify_combined). All tests run at both `-O0` and `-O1`.
- **Results**: All 126 portable tests pass (165 unit, 1286 valid, 261 invalid, 126 integration, 50 print-AST, 100 print-tokens, 21 print-asm). 6 new integration tests added; 120 existing tests continue to pass.
- **Self-host**: C0→C1→C2 verified with all tests passing at every stage. No source changes needed during bootstrap.
- **Version**: `src/version.c` bumped to `"01470000"`.
