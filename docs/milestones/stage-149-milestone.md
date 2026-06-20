# Milestone Summary

## Stage 149: Conditional Expression Folding - Complete

stage-149 extends the stage-142 optimizer with **conditional expression folding**: when the condition of an `?:` node is an `AST_INT_LITERAL` (after bottom-up optimization), the entire ternary expression is replaced with the selected branch and the dead branch is freed.

- **Optimizer**: New rule block in `src/optimize.c` within `optimize_expr`, inserted after the stage-147 boolean/logical simplification block and before the final `return node;`. Detects `AST_CONDITIONAL_EXPR` whose condition child is `AST_INT_LITERAL`. Nulls the kept branch's child slot before `ast_free` to prevent double-free, then returns the branch directly.
- **Folding rule**: `nonzero ? T : F → T` (frees `F` and the node); `0 ? T : F → F` (frees `T` and the node). No new node types or parser changes. Composition with prior folding rules (stage 143 binary folding, stage 144 unary folding) is automatic via bottom-up traversal.
- **Tests**: Four new integration tests (test_cond_fold_true, test_cond_fold_false, test_cond_fold_nested, test_cond_fold_combined), all using `-O1`. All existing tests pass at both `-O0` and `-O1`.
- **Results**: All 2016 portable tests pass (165 unit, 1286 valid, 261 invalid, 133 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Integration suite expanded from 129 to 133 tests.
- **Self-host**: C0→C1→C2 verified with all tests passing at every stage. No source changes needed during bootstrap.
- **Version**: `src/version.c` bumped to `"01490000"`.
