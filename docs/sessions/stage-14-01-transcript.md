# stage-14-01 Transcript: String Literal Tokens

## Summary

This stage adds lexer-level support for double-quoted string
literals. A new token type `TOKEN_STRING_LITERAL` is recognized when
the lexer encounters a `"`; the body bytes (without surrounding
quotes) are stored in `token.value` and the byte count in a new
`int length` field on `Token`. The `length` field is populated for
every token kind, not just strings — non-string paths run through a
small `finalize()` helper that sets `length = strlen(value)`, and
the string-literal path sets it explicitly so the count remains
correct once future stages introduce embedded NUL bytes via escape
sequences. Only ordinary characters are accepted this stage; the
body length is capped at 255 bytes to fit `Token.value[256]`.

Three lexer-level rejections are wired in, each with a distinct
diagnostic the invalid-test harness can match against: end-of-file
before the closing quote (`unterminated string literal`), a literal
newline byte before the closing quote (`newline in string literal`),
and any backslash inside a string body
(`escape sequences not supported in string literals`).

No parser, AST, codegen, or grammar changes — those are explicitly
out of scope for this stage.

## Changes Made

### Step 1: Tokenizer

- `include/token.h`: added `TOKEN_STRING_LITERAL` to the
  `TokenType` enum (after `TOKEN_INT_LITERAL`).
- `include/token.h`: added `int length;` to the `Token` struct.
- `src/lexer.c`: added a `static Token finalize(Token token)`
  helper that sets `token.length = (int)strlen(token.value)` and
  returns. Routed every non-string-literal `return token;` through
  it so all existing token kinds populate `length`.
- `src/lexer.c`: added a new `c == '"'` branch to
  `lexer_next_token` — consumes the opening quote, scans body
  bytes into `token.value`, rejects EOF / newline / backslash with
  the three diagnostics, enforces a 255-byte body cap, consumes
  the closing quote, sets `token.length = i` and
  `token.type = TOKEN_STRING_LITERAL`, and returns directly (not
  through `finalize`).

### Step 2: Compiler Driver

- `src/compiler.c`: added the `TOKEN_STRING_LITERAL` case to
  `token_type_name()` so `--print-tokens` renders the new type.
  Print-tokens output format is otherwise unchanged.

### Step 3: Tests — print_tokens

- `test/print_tokens/test_token_string_literal.c` containing
  `"Hello"`, `"abc"`, and `""` on three lines.
- Generated `test_token_string_literal.expected` from the new
  compiler mode and verified visually.

### Step 4: Tests — invalid

- `test_invalid_43__unterminated_string_literal.c` — file ends
  mid-string with no trailing newline so the lexer hits `\0`
  before any `"` or `\n`.
- `test_invalid_44__newline_in_string_literal.c` — string body
  crosses a literal newline.
- `test_invalid_45__escape_sequences_not_supported.c` — string
  body contains `\n`.

## Final Results

All four suites green:

- Valid:        232 / 232
- Invalid:       44 /  44   (+3)
- Print-AST:     19 /  19
- Print-Tokens:  72 /  72   (+1)
- **Aggregate:  367 / 367**, no regressions.

## Session Flow

1. Read the stage spec and the supporting `notes.md`,
   `transcript-format.md`, and `stage-kickoff-template.md`.
2. Reviewed `include/token.h`, `src/lexer.c`, `src/compiler.c`,
   the existing print_tokens harness, and the invalid-test runner
   for naming and matching conventions.
3. Wrote the kickoff document; flagged the spec-example
   case-typo and asked about length-field placement, max-length,
   and invalid-test scope.
4. User confirmed: new `int length` field on `Token`, populate
   for all token kinds, 255-byte cap, no valid tests this stage.
5. Updated `include/token.h`, refactored `src/lexer.c` to route
   every existing return path through a `finalize()` helper, and
   added the string-literal branch with three rejection paths
   and the 255-byte cap.
6. Built; ran the existing four-suite runner to confirm zero
   regressions before adding new tests.
7. Added the print_tokens happy-path test and the three invalid
   tests; generated the `.expected` file and confirmed each
   diagnostic fires with the expected fragment.
8. Re-ran the full suite (367/367), wrote the milestone and
   transcript, and committed.

## Notes

- The `finalize()` refactor is only used on existing token kinds
  whose body cannot contain NUL bytes today, so `strlen(value)`
  matches the source-byte count. The string-literal branch
  intentionally bypasses `finalize()` so once escape sequences
  introduce embedded NULs, `length` stays correct without further
  refactor.
- No grammar change in this stage — there is still no parser-
  level production for string literals.
