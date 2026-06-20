# Milestone Summary

## Stage 151: sizeof Constant Folding - Complete

Stage 151 adds sizeof constant folding to the AST-level optimizer: `AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` nodes are replaced with `AST_INT_LITERAL` at `-O1` wherever the size is determinable without runtime symbol-table access.

- **Optimizer**: Two new static helpers added to `src/optimize.c` before `optimize_expr`: `sizeof_scalar_size(TypeKind)` maps scalar type kinds to byte sizes (mirrors codegen but adds explicit `TYPE_DOUBLE: 8` case, fixing a latent bug where `sizeof(double)` returned 4 at `-O0`); `make_sizeof_literal(int)` creates an unsigned `TYPE_LONG` `AST_INT_LITERAL` with caller-allocated string. Two folding blocks inserted before `return node` in `optimize_expr`: (1) `AST_SIZEOF_TYPE` — always foldable, looks at `node->decl_type` and `node->full_type->size` for struct/union/array, calls `sizeof_scalar_size` for scalars; (2) `AST_SIZEOF_EXPR` — partial fold for string-literal (returns `byte_length+1`), int-literal (uses `child->decl_type`), char-literal (returns 4); variable references and complex expressions fall through unchanged.
- **Grammar/Parser**: No changes.
- **AST/Semantics**: One bug found and fixed during implementation: `make_sizeof_literal` originally passed stack `buf` pointer to `ast_new`, which stores it directly without copying. Fixed with `util_strdup(buf)`.
- **Codegen**: No changes. Existing codegen handlers for `AST_SIZEOF_TYPE` / `AST_SIZEOF_EXPR` remain as fallback.
- **Tests**: 5 new integration tests (all with `-O1` .cflags): test_sizeof_type_fold, test_sizeof_expr_fold, test_sizeof_dead_branch, test_sizeof_string_fold, test_sizeof_struct_fold. Full suite 2027/2027 tests pass (165 unit, 1286 valid, 261 invalid, 144 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- **Docs**: Kickoff and spec committed in prior phase; checklist updated with Stage 151 section; self-compilation report updated with stage-151 C0→C1→C2 run; `VERSION_STAGE` bumped to "01510000" in `src/version.c`.
- **Notes**: All folding gated behind `-O1`. After folding, sizeof values compose freely with stages 143–150 (e.g., `sizeof(long) == 8` triggers dead-branch elimination via stage 150). Self-host C0→C1→C2 verified with all tests passing at every stage.
