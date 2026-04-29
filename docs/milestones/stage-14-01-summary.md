# Stage-14-01 Milestone: String Literal Tokens

## Completed

- Added `TOKEN_STRING_LITERAL` to `TokenType` and a new `int length`
  field on `Token`. Every token returned by `lexer_next_token` now
  populates `length` with the byte count of the token's source
  text (for non-string tokens this equals `strlen(value)`, computed
  via a single `finalize()` helper; for string literals the lexer
  sets `length` explicitly so it stays correct once future stages
  introduce embedded NUL bytes).
- Lexer recognizes a double-quoted string literal as a single
  token. The body bytes (without the surrounding quotes) are stored
  in `token.value` and the byte count in `token.length`. Only
  ordinary characters are accepted in this stage. Body length is
  capped at 255 bytes to fit `Token.value[256]`.
- Three lexer-level rejection paths, each with a distinct
  diagnostic:
  - `error: unterminated string literal` — EOF before closing
    quote.
  - `error: newline in string literal` — literal newline byte
    before closing quote.
  - `error: escape sequences not supported in string literals` —
    any backslash inside a string body.
- `--print-tokens` driver knows the new type via the
  `token_type_name()` mapping; print-tokens output format is
  unchanged.
- No parser, AST, codegen, or grammar changes (strictly out of
  scope this stage).

## Files Changed

- `include/token.h` — added `TOKEN_STRING_LITERAL`; added
  `int length` field on `Token`.
- `src/lexer.c` — added `finalize()` helper that sets
  `token.length` on every non-string return path; added the
  double-quoted string-literal branch with three rejection
  diagnostics and the 255-byte body cap.
- `src/compiler.c` — added `TOKEN_STRING_LITERAL` to
  `token_type_name()`.
- `test/print_tokens/test_token_string_literal.c` (+`.expected`)
  — exercises `"Hello"`, `"abc"`, and the empty string `""`.
- `test/invalid/test_invalid_43__unterminated_string_literal.c`
  — file ends mid-literal with no trailing newline.
- `test/invalid/test_invalid_44__newline_in_string_literal.c`
  — string body crosses a literal newline.
- `test/invalid/test_invalid_45__escape_sequences_not_supported.c`
  — string body contains `\n` escape.

## Test Results

- Valid:        232 / 232 pass.
- Invalid:       44 /  44 pass (+3).
- Print-AST:     19 /  19 pass.
- Print-Tokens:  72 /  72 pass (+1).
- Aggregate:    367 / 367 pass. No regressions.
