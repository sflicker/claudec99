# Milestone Summary

## Stage 150: Dead-Branch Elimination - Complete

Stage 150 extends the AST-level optimizer with dead-branch elimination for `if`, `while`, and `for` statements whose controlling condition is a compile-time integer constant.

- **Optimizer**: Dead-branch elimination rules added to `optimize_stmt` in `src/optimize.c` for three statement types. When a condition folds to `AST_INT_LITERAL` after child optimization: `if(nonzero)` returns the then-branch, `if(0)` returns the else-branch or empty block, `while(0)` returns empty block (non-zero `while` not eliminated to preserve infinite loops), `for(init;0;update)` returns init only (or empty block if no init). All rules null dead-child slots before `ast_free` to prevent double-free.
- **Grammar/Parser**: No changes.
- **AST/Semantics**: Memory management follows pattern from stages 145/149: detach kept nodes, free parent and dead branches.
- **Codegen**: No changes. One bug discovered during development: for-statement init expressions (e.g., `AST_ASSIGNMENT`) must be wrapped in `AST_EXPRESSION_STMT` when lifted into the parent block's codegen flow (codegen handles expression inits via `codegen_expression()`, not `codegen_statement()`).
- **Tests**: 6 new integration tests (all with `-O1` .cflags): test_dead_if_true, test_dead_if_false, test_dead_while, test_dead_for, test_dead_for_no_init, test_dead_combined. Full suite 2022/2022 tests pass (165 unit, 1286 valid, 261 invalid, 139 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- **Docs**: Kickoff and spec committed in prior phase; checklist updated with Stage 150 section; self-compilation report updated with stage-150 C0→C1→C2 run; `VERSION_STAGE` bumped to "01500000" in `src/version.c`.
- **Notes**: Self-host C0→C1→C2 verified with no source changes needed. All tests pass at every stage.
