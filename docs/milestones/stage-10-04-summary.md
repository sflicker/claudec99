# Stage-10-04 Milestone: goto Statement

Stage 10-04 adds support for C's `goto` statement and the identifier
form of `<labeled_statement>`. A `goto <identifier>;` transfers control
to the matching `<identifier>:` label within the same function.

## Completed

- Tokenizer recognizes the `goto` keyword (`TOKEN_GOTO`).
- Parser accepts labeled statements of the form `<identifier> ":" <statement>`
  and the jump statement `"goto" <identifier> ";"`.
- AST gained two nodes: `AST_LABEL_STATEMENT` (value = label name,
  single child = the labeled statement) and `AST_GOTO_STATEMENT`
  (value = target label name).
- Pretty printer emits `LabelStmt: <name>` and `GotoStmt: <name>`.
- Code generator collects user labels per-function before emission,
  rejects duplicates and unknown goto targets, and emits labels as
  `.L_usr_<func>_<name>` so names cannot collide across functions.
- Documentation: `docs/grammar.md` updated with the new labeled and
  jump-statement productions.
- Tests: the three provided valid goto tests pass; added
  `test_invalid_13__duplicate_label.c`,
  `test_invalid_14__undefined_label.c`, and a `test_goto` print_ast
  test. Full suite — 121 valid, 14 invalid, 10 of 11 print_ast (the
  single failure is a pre-existing `test_switch` regression unrelated
  to this stage).
