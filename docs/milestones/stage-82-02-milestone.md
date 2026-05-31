# Milestone Summary

## Stage 82-02: const-Qualified Member Lvalue Rules - Complete

stage-82-02 enforces const-qualification rules for array subscript writes through struct members, rejecting writes via subscript to pointer-to-const members (e.g., `s.p[0] = 'H'` where `s.p: const char *`).

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: In `src/codegen.c` at the `AST_ASSIGNMENT + AST_ARRAY_INDEX` path, after `emit_array_index_addr`, check if the returned element type is const-qualified and emit "assignment through pointer to const-qualified type" diagnostic error if so.
- Tests: 2 new tests added (1 valid, 1 invalid). Full suite 1251 passed, 0 failed.
- Docs: Updated README.md stage reference and const qualifier feature bullet. Updated test totals.
- Notes: Subscript writes through const pointers were previously undetected; dereference writes (`*s.p = 'H'`) were already caught by stage-66. The subscript path required a dedicated check since `emit_array_index_addr` returns the element type without immediate const validation. Error message reuses existing "assignment through pointer to const-qualified type" string for consistency.
