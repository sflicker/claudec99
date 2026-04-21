# Stage-10-05 Milestone: Simple Comments

Stage 10-05 adds lexical support for C-style comments. The tokenizer
now silently skips line comments (`// ... <end of line>`) and block
comments (`/* ... */`, possibly multi-line), so later stages never
observe them.

## Completed

- Tokenizer's whitespace-skip routine extended to also consume `//`
  and `/* ... */` comments, looping until neither whitespace nor a
  comment is pending.
- No parser, AST, or codegen changes — comments are purely lexical.
- No grammar changes required in `docs/grammar.md`.
- Tests: added three valid tests exercising line comments, inline
  block comments, and multi-line block comments. Full suite — 124
  valid (121 previous + 3 new), 14 invalid, 11 print_ast, all passing
  with no regressions.
