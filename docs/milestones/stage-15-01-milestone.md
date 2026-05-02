# Milestone Summary

## Stage-15-01: Character literal lexer support — Complete

- Added `TOKEN_CHAR_LITERAL` to the token enum and a new char-literal
  branch in the lexer. Supported forms: `'A'`, `'0'`, and the escape
  set `\n`, `\t`, `\r`, `\\`, `\'`, `\"`, `\0`. Each token carries
  the decoded byte at `value[0]` (length 1), the evaluated integer at
  `long_value`, and `literal_type = TYPE_INT`.
- Rejected forms emit explicit lexer diagnostics:
  - empty character literal (`''`),
  - unterminated character literal (no closing quote before line end
    or EOF),
  - multi character constant (`'ab'`, `'\nA'`, etc.),
  - invalid escape sequence in character literal (`'\x41'`,
    `'\123'`, `'\q'`),
  - wide character literal not supported (`L'A'`, `U'A'`, `u'A'`).
- Extended `--print-tokens` output: `TOKEN_CHAR_LITERAL` rows reuse
  the string-literal re-escape logic so escape forms print legibly,
  and the re-escape table now also escapes `'` so `'\''` displays as
  `\'`.
- Tests added:
  - 1 print_tokens test covering all nine spec forms.
  - 7 invalid tests (`56`–`62`) covering each rejection case.
- Parser, AST, and code generator are unchanged this stage; the new
  token is not yet consumed by `<primary_expression>`.
- Documentation updated: `docs/grammar.md` records the lexer-level
  rule for `<character_literal>` and `<character_escape_sequence>`;
  `README.md` bumped to "Through stage 15-01" with new test totals
  (417 = 252 valid + 60 invalid + 23 print-AST + 74 print-tokens +
  8 print-asm).
- Full suite: 417/417 pass; no regressions.
