# stage-15-01 Transcript: Character Literal Lexer Support

## Summary

Stage 15-01 adds lexer-only support for character literal tokens. The
lexer recognizes single-quoted single-byte literals using the same
escape set already accepted in string literals, plus the new `\'`
form, and emits a single `TOKEN_CHAR_LITERAL` for each. Each token
carries the decoded byte at `value[0]`, the evaluated integer at
`long_value`, and `literal_type = TYPE_INT` (matching C's rule that
character constants have type `int`).

Rejected forms — empty literals, unterminated literals, multi
character constants, unsupported escapes (octal, hex, single-letter
unknown), and wide/unicode prefixes (`L'...'`, `U'...'`, `u'...'`) —
each produce a dedicated lexer diagnostic. Wide-prefix rejection is
performed before the identifier branch can consume the prefix
character.

Parser, AST, and code generator are unchanged. The grammar file
records the lexer-level shape of the new token but does not yet add
it to `<primary_expression>`.

## Plan

The plan was to add a `TOKEN_CHAR_LITERAL` enum value, a single new
branch in `lexer_next_token`, an explicit wide-char rejection
preceding the identifier branch, and corresponding entries in the
compiler driver's token-name table and re-escape printer. Tests cover
the nine spec forms via `print_tokens` and seven rejection cases via
`invalid`. Documentation: lexer-level rule in `docs/grammar.md` and a
features bullet plus updated test totals in `README.md`.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_CHAR_LITERAL` to `TokenType` in `include/token.h`.
- Added a `'\''` branch in `src/lexer.c` `lexer_next_token` that
  decodes the inner character or escape, stores the decoded byte at
  `token.value[0]` (length 1, NUL sentinel at `value[1]`), records
  the evaluated integer at `token.long_value`, sets
  `token.literal_type = TYPE_INT`, and emits the token.
- Added explicit lexer diagnostics for `''` (empty), missing closing
  quote (unterminated), more than one byte before the closing quote
  (multi character constant), and any escape outside the supported
  set (invalid escape sequence in character literal).
- Added a guard immediately before the identifier branch that
  rejects `L'...'`, `U'...'`, and `u'...'` with the diagnostic
  `wide character literal not supported`.

### Step 2: Parser

- No changes (lexer-only stage).

### Step 3: AST

- No changes.

### Step 4: Code Generator

- No changes.

### Step 5: Compiler Driver

- Added `TOKEN_CHAR_LITERAL` to `token_type_name` in
  `src/compiler.c`.
- Extended `print_token_row`'s re-escape branch to also handle
  `TOKEN_CHAR_LITERAL` and added a `case '\''` that prints `\'`.

### Step 6: Tests

- Added `test/print_tokens/test_token_char_literal.c` and
  `test_token_char_literal.expected` covering all nine spec forms
  (`'A'`, `'0'`, `'\n'`, `'\t'`, `'\r'`, `'\\'`, `'\''`, `'\"'`,
  `'\0'`).
- Added `test/invalid/test_invalid_56__unterminated_character_literal.c`
  exercising `'a;` (no closing quote on the line).
- Added `test/invalid/test_invalid_57__empty_character_literal.c`
  exercising `''`.
- Added `test/invalid/test_invalid_58__multi_character_constant.c`
  exercising `'ab'`.
- Added `test/invalid/test_invalid_59__invalid_escape_sequence.c`
  exercising `'\x41'`.
- Added `test/invalid/test_invalid_60__wide_character_literal.c`
  exercising `L'A'`.
- Added `test/invalid/test_invalid_61__wide_character_literal.c`
  exercising `U'A'`.
- Added `test/invalid/test_invalid_62__wide_character_literal.c`
  exercising `u'A'`.

### Step 7: Documentation

- `docs/grammar.md`: added `<character_literal> ::= TOKEN_CHAR_LITERAL`
  with a comment noting it is lexer-only this stage, plus a
  `<character_escape_sequence>` rule that adds `\'` to the existing
  string escape set.
- `README.md`: bumped the "Through stage" line from 14-08 to 15-01,
  added a features bullet for lexer-only character literal support,
  and updated suite totals from 409 (53 invalid, 73 print-tokens) to
  417 (60 invalid, 74 print-tokens).

## Final Results

All 417 tests pass: 252 valid + 60 invalid + 23 print-AST + 74
print-tokens + 8 print-asm. No regressions.

## Session Flow

1. Read the stage-15-01 spec and reviewed the existing string-literal
   lexer path, token struct, and print-tokens machinery to settle on
   a representation.
2. Flagged ambiguity around "original spelling" storage and the
   wide-char `can be rejected` allowance; the user requested explicit
   lexer-stage rejection of `L`/`U`/`u` prefixes plus dedicated
   invalid tests.
3. Wrote the kickoff document and presented the plan.
4. Implemented the tokenizer change, the wide-char rejection, the
   driver-side token name and re-escape, and built clean.
5. Smoke-tested all nine spec forms and all seven rejection cases.
6. Refined multi character vs. unterminated disambiguation by
   scanning ahead for a closing quote.
7. Added the print-tokens and invalid tests, observed one
   filename-fragment mismatch (`multi-character` vs. `multi
   character`), and dropped the hyphen from the diagnostic to align
   with the underscore-to-space matcher.
8. Ran the full test suite and confirmed 417/417 pass.
9. Updated `docs/grammar.md` and `README.md`, then wrote the
   milestone and this transcript.

## Notes

- The token records both the decoded byte and the integer value;
  this duplicates information but keeps the existing print path's
  re-escape logic working unchanged for the new token kind.
- Wide-prefix rejection is placed immediately before the identifier
  branch because `L`, `U`, and `u` are valid identifier starts; this
  is the only point where the prefix is unambiguously paired with a
  following `'`.
- The diagnostic wording avoids hyphens (`multi character constant`)
  to keep the filename-based fragment matcher in
  `test/invalid/run_tests.sh` aligned without renaming files.
