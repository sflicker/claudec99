# Milestone Summary

## Stage 160: sizeof(expr) in Constant Expressions and SDL2 Integration Test - Complete

stage-160 ships `sizeof(expression)` in constant expression contexts (enabling compile-time size queries through struct-member access via casts) and establishes optional-library test infrastructure with an SDL2 integration test.

- Tokenizer: No changes.
- Grammar/Parser: `eval_const_primary` in `src/parser.c` extended to parse and evaluate expressions when `sizeof` is followed by a non-type-name token; both `full_type` (when available) and scalar `decl_type` fallback paths resolve member sizes.
- AST/Semantics: `AST_SIZEOF_EXPR` in `src/codegen.c` gains a cast-based arrow-access case to resolve field sizes from pointer types created via cast expressions.
- Codegen: No structural changes; existing sizeof infrastructure reused.
- Tests: 2 new portable sizeof integration tests (`test_sizeof_expr_ptr`, `test_sizeof_expr_array_dim`), 1 new optional-library test (`test_sdl2_init` skipped when SDL2 absent), .require companion file support in new `test/integration_sysinclude/` suite. Full suite: 2063 portable (2061 + 2) + 180 system-include + 1 optional-library, all pass.
- Docs: README updated with test counts, optional-library section, and sizeof notes; checklist updated; new `test/integration_sysinclude/run_tests.sh` runner with `.require` skip mechanism; existing `run_tests_sysinclude.sh` updated for .require check; `run_all_tests.sh` wired to invoke optional-library suite on Linux x86_64.
- Notes: Self-host C0→C1→C2 verified with all tests passing at every stage; no source changes needed during bootstrap. Stage 160 resolves SDL2 `sizeof(((SDL_Event *)NULL)->padding)` pattern and establishes reusable infrastructure for future optional-library integration tests (zlib, OpenGL, etc.).
