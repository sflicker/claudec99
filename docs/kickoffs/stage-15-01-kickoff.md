# Stage-15-01 Kickoff

## Spec
`docs/stages/ClaudeC99-spec-stage-15-01-character-literal-lexer-support.md`

## Stage Goal
Add lexer-only support for character literal tokens. The lexer must
emit a single `TOKEN_CHAR_LITERAL` for each `'...'` literal containing
both the original spelling (preserved via re-escaping at display time,
matching the existing string-literal convention) and the evaluated
integer value.

Supported forms: `'A'`, `'0'`, `'\n'`, `'\t'`, `'\r'`, `'\\'`,
`'\''`, `'\"'`, `'\0'`. Out-of-scope forms (multi-character constants,
octal/hex escapes, wide/unicode prefixes) may be rejected.

## Delta from Stage 14-08
Stage 14-08 was tests-only. Stage 15-01 is the first lexer-only
addition for a new literal kind, analogous to stage 14-02 (string
literal lexer support) but for `char` literals.

## Ambiguities Flagged
- **"Original spelling" storage.** The spec asks the token to carry
  both the original spelling and the evaluated integer value. The
  existing string-literal token stores decoded bytes in `token.value`
  and the print path re-escapes them for display; the same convention
  is used here (raw decoded byte at `value[0]`, length 1, integer in
  `long_value`).
- **Wide-char prefixes (`L'A'` etc.) listed as "can be rejected".**
  These will be rejected explicitly at the lexer with a dedicated
  diagnostic, before the identifier branch can consume `L`/`U`/`u`.
- **Backtick typo in the spec.** Line 41 reads `` `\x41' `` (opening
  backtick instead of opening single quote). The intent is clearly
  the hex-escape case alongside `'\123'`. No effect on implementation.

## Planned Changes

### Tokenizer (`include/token.h`, `src/lexer.c`)
- Add `TOKEN_CHAR_LITERAL` to the `TokenType` enum.
- New `'\''` branch in `lexer_next_token`:
  - Read one inner character (or one supported `\` escape).
  - Reject empty `''`, newline/EOF before the closing quote,
    multi-character constants, and unsupported escapes (including
    octal and hex forms).
  - Decode into `token.value[0]`; set `token.length = 1`,
    `token.long_value = (unsigned char)decoded`,
    `token.literal_type = TYPE_INT`,
    `token.type = TOKEN_CHAR_LITERAL`.
- Reject wide-character literals (`L'...'`, `U'...'`, `u'...'`)
  before the identifier branch with a dedicated diagnostic.

### Parser (`src/parser.c`)
- No changes (lexer-only stage).

### AST
- No changes.

### Code Generator (`src/codegen.c`)
- No changes.

### Compiler driver (`src/compiler.c`)
- Add `TOKEN_CHAR_LITERAL` to `token_type_name`.
- Extend `print_token_row`'s re-escape branch to apply to char
  literals as well, so `'\n'` displays as `\n`, etc.

### Tests
- `test/print_tokens/test_token_char_literal.c` plus
  `.expected` covering simple and escape forms.
- `test/invalid/test_invalid_56__unterminated_character_literal.c`
- `test/invalid/test_invalid_57__empty_character_literal.c`
- `test/invalid/test_invalid_58__multi_character_constant.c`
- `test/invalid/test_invalid_59__invalid_escape_sequence.c`
  (`'\x41'`, sharing the existing "invalid escape sequence" diagnostic
  family).
- `test/invalid/test_invalid_60__wide_character_literal.c` (`L'A'`).
- `test/invalid/test_invalid_61__wide_character_literal.c` (`U'A'`).
- `test/invalid/test_invalid_62__wide_character_literal.c` (`u'A'`).

### Documentation
- `docs/grammar.md`: add a `<character_literal>` lexical rule and the
  supported escape set. The rule is lexer-level; `<primary_expression>`
  is intentionally not extended this stage because the parser does not
  yet consume `TOKEN_CHAR_LITERAL`.
- `README.md`: bump "Through stage 14-08" to "Through stage 15-01";
  add a bullet for character literal tokenization; update the
  aggregate test totals.

### Commit
Single commit at the end of the stage.
