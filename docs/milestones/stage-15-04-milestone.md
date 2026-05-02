# Milestone Summary

## Stage-15-04: Character literal escape and diagnostic completion — Complete

- Filled in the remaining simple character-literal escapes in the
  lexer: `\a` (7), `\b` (8), `\f` (12), `\v` (11), `\?` (63). The
  previously-supported escapes (`\n`, `\t`, `\r`, `\\`, `\'`, `\"`,
  `\0`) were left unchanged, per the spec instruction to "leave
  alone what already works".
- Split the character-literal early-exit branch so a raw newline
  produces `error: newline in character literal` instead of being
  reported as `unterminated character literal`. EOF before any byte
  continues to report unterminated. Empty `''`, multi-character
  `'ab'`, and unknown escapes such as `'\q'` continue to produce
  their existing distinct diagnostics.
- No parser, AST, or code-generator changes were required —
  character literals already flow through the existing
  `AST_CHAR_LITERAL` path with `decl_type = TYPE_INT`.
- Added 5 new valid tests (`test/valid/test_char_literal_escape_a__7.c`,
  `..._b__8.c`, `..._f__12.c`, `..._v__11.c`,
  `..._question__63.c`) covering the new escape values.
- Added 3 new invalid tests
  (`test/invalid/test_invalid_63__invalid_escape_sequence.c` for
  `'\q'`,
  `test_invalid_64__unterminated_character_literal.c` for `'\'`, and
  `test_invalid_65__newline_in_character_literal.c` for a raw
  newline inside the literal).
- Octal `\NNN` (other than the special `\0`), hex `\xNN`,
  multi-character constants, and wide-character literals
  (`L'A'`, `u'A'`, `U'A'`) remain rejected as out of scope.
- Documentation: `docs/grammar.md` `<character_escape_sequence>`
  production extended to include the new escapes; `README.md`
  bumped to "Through stage 15-04" with the character-literal bullet
  and aggregate test totals refreshed; new
  `status/project-features-through-stage-15-04.md` and
  `status/project-status-through-stage-15-04.md` snapshots created
  (existing pre-12-04 status files left alone).
- Full suite: 443/443 pass (274 valid + 63 invalid + 24 print-AST +
  74 print-tokens + 8 print-asm). No regressions from prior stages.
