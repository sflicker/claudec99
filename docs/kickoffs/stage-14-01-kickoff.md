# Stage-14-01 Kickoff: String Literal Tokens

## Spec Summary

Stage 14-01 teaches the lexer to recognize a double-quoted string
literal as a single token of a new type `TOKEN_STRING_LITERAL`. The
token records the bytes between the surrounding quotes plus an
explicit byte length. Only ordinary characters are accepted in this
stage. The lexer must reject — with a clear diagnostic — three
cases: a literal newline byte before the closing quote, end-of-file
before the closing quote, and any backslash escape sequence.

Out of scope for this stage: parser, AST, codegen, escape sequences,
adjacent-literal concatenation.

## What Must Change vs. Stage 14-00

  - `include/token.h`: add `TOKEN_STRING_LITERAL` to `TokenType`; add
    a new `int length` field on `Token` (byte length of the token's
    text bytes — for every token type, not just strings).
  - `src/lexer.c`: populate `token.length` for every token returned
    by `lexer_next_token`; add a `"`-prefixed branch that scans
    ordinary characters, emits the three required diagnostics, and
    returns `TOKEN_STRING_LITERAL`.
  - `src/compiler.c`: extend `token_type_name()` so `--print-tokens`
    can render the new type.
  - `test/print_tokens/`: add at least one happy-path test exercising
    `TOKEN_STRING_LITERAL`.
  - `test/invalid/`: add three `test_invalid_<n>__<fragment>.c`
    files, one per rejection case (unterminated EOF, newline in
    string, escape sequence).
  - `docs/grammar.md`: no change. There is no parser-level grammar
    addition this stage.

## Decisions (Confirmed)

  1. **`Token.length` is a new dedicated `int` field** populated for
     every token kind, equal to the byte length of the token's
     text. For string literals, the length counts only the bytes
     between the quotes (the stored byte array). For ordinary
     tokens, length equals the number of source bytes the token
     spans (which equals `strlen(value)` for the current token
     types where no embedded NULs exist).
  2. **Max string-literal body length: 255 bytes.** Strings whose
     body exceeds 255 bytes are rejected with a clear diagnostic,
     matching the integer-literal-too-large pattern already used by
     the lexer.
  3. **Stored byte array preserves case.** The spec example "Hello
     -> hello" is read as a typo. The byte array is copied verbatim.
  4. **`--print-tokens` rendering** is unchanged in format. The
     value column shows the raw bytes (no quotes) using the existing
     18-char left-justified column. The new `length` field does not
     appear in print-tokens output, which keeps every existing
     `.expected` file in `test/print_tokens/` valid.
  5. **Diagnostics**: `fprintf(stderr, ...); exit(1);` matching the
     existing integer-overflow path. Each of the three rejection
     cases gets a distinct fragment that the invalid-test runner can
     match (case-insensitive, underscores-as-spaces).
  6. **Invalid test naming**: `test_invalid_<N>__<fragment>.c` with
     the fragment matching a substring of the compiler's stderr
     diagnostic. Numbers 43, 44, 45 (next free).
  7. **No new "valid" tests this stage.** The parser/codegen do not
     understand string literals yet, so a string in a normal program
     would fail to parse. Coverage is print_tokens + invalid only.
  8. **No grammar.md change.** Tooling-and-lexer-only stage.

## Spec Issues Flagged

  - **Spec example typo.** "`\"Hello\" -> hello, length 5`" is read
    as a typo. The stored byte array preserves case (`Hello`).
  - **Test plan scope.** Spec says "Add a print_tokens test"
    (singular). Per user direction, also adding three invalid tests
    for the rejection cases.

## Planned Changes

  1. **Tokenizer** —
     - `include/token.h`: add `TOKEN_STRING_LITERAL`; add
       `int length` to `Token`.
     - `src/lexer.c`: set `token.length` on every return path;
       add a `"`-prefixed branch that scans ordinary bytes, rejects
       newline / EOF / backslash with distinct diagnostics, enforces
       the 255-byte body cap, copies the inner bytes into
       `token.value`, sets `token.length`, and returns
       `TOKEN_STRING_LITERAL`.
  2. **Parser** — none.
  3. **AST** — none.
  4. **Code generator** — none.
  5. **Compiler driver (`src/compiler.c`)** — add the
     `TOKEN_STRING_LITERAL` case to `token_type_name()`.
  6. **Tests** —
     - `test/print_tokens/test_token_string_literal.c` and matching
       `.expected` (a small file containing one or more string
       literals).
     - `test/invalid/test_invalid_43__unterminated_string_literal.c`
       (EOF before closing quote).
     - `test/invalid/test_invalid_44__newline_in_string_literal.c`
       (newline byte before closing quote).
     - `test/invalid/test_invalid_45__escape_sequences_not_supported.c`
       (a backslash escape inside a string literal).
  7. **Grammar** — no change.
  8. **Docs** — kickoff (this), milestone, session transcript.
  9. **Commit** — single commit when all suites are green.
