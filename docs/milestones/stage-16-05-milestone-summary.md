## Milestone Summary

Stage-16-05: Remaining compound assignment operators — Complete

stage-16-05 ships the eight remaining compound assignment operators:
`*=`, `/=`, `%=`, `<<=`, `>>=`, `&=`, `^=`, `|=`.

- Tokenizer: 8 new token constants (`TOKEN_STAR_ASSIGN`, `TOKEN_SLASH_ASSIGN`,
  `TOKEN_PERCENT_ASSIGN`, `TOKEN_LEFT_SHIFT_ASSIGN`, `TOKEN_RIGHT_SHIFT_ASSIGN`,
  `TOKEN_AMP_ASSIGN`, `TOKEN_CARET_ASSIGN`, `TOKEN_PIPE_ASSIGN`). Lexer updated
  for `*`, `/`, `%` (single lookahead for `=`), `<<`/`>>` (third-char lookahead
  for `=`), and `&`, `|`, `^` (second-char lookahead for `=`). `--print-tokens`
  extended for all 8 new names.
- Grammar/Parser: `parse_expression` fast-path extended to recognise all 10
  compound-assignment tokens. Each `a op= b` desugars to `a = a op b` via the
  existing `AST_BINARY_OP` / `AST_ASSIGNMENT` mechanism. No new AST node types.
- AST: no changes.
- Codegen: no changes; desugared trees reuse existing arithmetic, shift, and
  bitwise paths.
- Tests: 16 new (8 valid, 8 print-tokens). Full suite 551/551 pass
  (329 valid + 91 invalid + 24 print-AST + 88 print-tokens + 19 print-asm).
  No regressions.
- Docs: `docs/grammar.md` updates `<assignment_operator>` to include all 10
  operators (fixing the `%=` omission and `&=` duplication in the spec grammar).
  `README.md` bumped to "Through stage 16-05" with a new compound-assignment
  bullet and refreshed test totals.
- Notes: Spec grammar had `&=` listed twice and omitted `%=`. The task section
  was authoritative; grammar corrected accordingly.

## Generated Artifacts

- docs/kickoffs/stage-16-05-kickoff.md
- docs/milestones/stage-16-05-milestone-summary.md
- 8 valid tests, 8 print-tokens test pairs
- Source changes: include/token.h, src/lexer.c, src/compiler.c, src/parser.c
- Doc updates: docs/grammar.md, README.md
- Commit: 590456b — stage-16-05: remaining compound assignment operators
