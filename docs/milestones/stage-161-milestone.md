# Milestone Summary

## Stage 161: void * Comparison with Typed Pointers - Complete

stage-161 fixes a C99 compatibility bug where comparing `FILE *` to `NULL` (expanded as `((void *)0)` in system headers) was rejected with "incompatible pointer types in comparison". The fix enables pointer comparison between `void *` and any typed object pointer, as permitted by C99 §6.5.9.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: One-line fix in `src/codegen.c` at the pointer comparison validation block: replaced `pointer_types_equal()` with `pointer_types_assignable()` for relational equality operators (`==`, `!=`). The `pointer_types_assignable()` function already permits `void *` on either side; `pointer_types_equal()` did not.
- Tests: 2 new portable integration tests (`test_void_ptr_cmp` for void pointer comparison, `test_null_file_cmp` for the exact bug scenario), 2 system-include tests (both also pass with real headers). Full suite: 2065 portable (2063 + 2) + 182 system-include (180 + 2), all pass.
- Docs: README updated with test counts; checklist updated with new stage entry.
- Notes: Self-host C0→C1→C2 verified with all tests passing at every stage; no source changes needed during bootstrap. The `pointer_types_assignable` function has been in the codebase since stage 38 and is already self-compilable.
