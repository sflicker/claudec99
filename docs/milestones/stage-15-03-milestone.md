# Milestone Summary

## Stage-15-03: Character literal type rules and integration — Complete

- Verified that `AST_CHAR_LITERAL` carries `decl_type = TYPE_INT`
  (parser) and that codegen reports `result_type = TYPE_INT` and
  emits `mov eax, <int>` — both already in place from stage 15-02.
  No tokenizer, parser, AST, or code-generator changes were needed.
- Added 7 integration tests under `test/valid/` covering every
  integer context the spec calls out:
  - `test_stage_15_03_char_literal_assign_char__65.c` — `char c = 'A';`
  - `test_stage_15_03_char_literal_assign_int__65.c`  — `int x = 'A';`
  - `test_stage_15_03_char_literal_assign_long__65.c` — `long x = 'A';`
  - `test_stage_15_03_char_literal_eq_int__1.c`       — `'A' == 65`
  - `test_stage_15_03_char_literal_if_truthy__7.c`    — `if ('A')`
  - `test_stage_15_03_char_literal_if_nul_falsy__3.c` — `if ('\0')`
  - `test_stage_15_03_char_literal_array_element_assign__72.c`
    — `char s[] = "hi"; s[0] = 'H'; return s[0];`
  The spec's `return 'A';` and `return 'A' + 1;` cases were already
  covered by stage 15-02 tests and were not duplicated.
- Confirmed assignment to a smaller integer type (`char c = 'A';`)
  uses the existing conversion / truncation path with no new code.
- Confirmed no special `char`-typed expression form was introduced
  for character literals — the constant remains typed `int`,
  matching C semantics.
- Spec issues called out before implementation: typo
  `int ('\0') {` (clearly meant `if ('\0') {`) and
  `AST_CHAR_INTERAL` (typo for `AST_CHAR_LITERAL`).
- Documentation: `docs/grammar.md` unchanged (the
  `<character_literal>` production and `int`-typed note were added
  in stage 15-02). `README.md` bumped to "Through stage 15-03",
  character-literal bullet expanded to enumerate the integer
  contexts, and aggregate test totals refreshed (435 = 269 valid +
  60 invalid + 24 print-AST + 74 print-tokens + 8 print-asm).
- Full suite: 435/435 pass; no regressions.
