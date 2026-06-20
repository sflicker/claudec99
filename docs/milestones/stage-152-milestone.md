# Milestone Summary

## Stage 152: Constant Propagation for `const` Scalar Locals - Complete

Stage 152 ships constant propagation for `const`-qualified scalar local variables with compile-time integer-literal initializers. At each `AST_VAR_REF`, the optimizer substitutes the recorded literal value, making the constant visible to subsequent folding passes (stages 143–151).

- **Tokenizer**: No changes.
- **Grammar/Parser**: No changes.
- **AST/Semantics**: No new node types or fields. The existing `is_const` field on `ASTNode` is used to identify eligible declarations.
- **Optimizer**: File-static `ConstEntry` table (`g_const_table[64]`) and count (`g_const_count`) added to `src/optimize.c` before `optimize_expr`. Three new helpers: `is_scalar_int_type(TypeKind)` (identifies eligible types), `const_prop_lookup(const char *name)` (right-to-left scope-aware lookup). In `optimize_expr`, `AST_VAR_REF` nodes are checked against the table and substituted with fresh `AST_INT_LITERAL` nodes carrying the declared type and signedness. In `optimize_stmt`, `AST_DECLARATION` case now records eligible const-scalar-literal locals; `AST_BLOCK` case saves/restores `g_const_count` for proper scope nesting. In `optimize_translation_unit`, `g_const_count` is reset to 0 before each function body. All propagation gated behind `-O1`.
- **Codegen**: No changes.
- **Tests**: 5 new integration tests (basic, fold, dead-branch, scope, init-fold) covering const substitution and interaction with stages 143 and 150. Full suite 2032/2032 portable tests pass (165 unit, 1286 valid, 261 invalid, 149 integration, 50 print-AST, 100 print-tokens, 21 print-asm) plus 149/149 system include tests.
- **Docs**: Kickoff, spec, and self-compilation report updated. `VERSION_STAGE` bumped to `"01520000"` in `src/version.c`.
- **Notes**: Scope nesting handled via save/restore pattern. Declarations must be eligible: `is_const && is_scalar_int_type(decl_type) && child[0] == AST_INT_LITERAL` (after optimization). Substitution composes with stage-143 binary folding and stage-150 dead-branch elimination (e.g., `const int N = 8; int a = N * 4;` folds to 32; `const int FLAG = 0; if (FLAG) {...}` eliminates the then-branch). Self-host C0→C1→C2 verified: all tests pass at every stage with no source changes needed during bootstrap.
