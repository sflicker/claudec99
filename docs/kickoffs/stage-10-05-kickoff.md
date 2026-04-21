# Stage-10-05 Kickoff: Simple Comments

## 1. Summary of the spec

Stage 10.5 adds support for two kinds of C comments:

- Line comments: `// ... <end of line>`
- Block comments: `/* ... */`, which may span multiple lines.

The tokenizer must silently skip comments so later stages (parser,
AST, codegen) never see them.

## 2. What must change from the previous stage

| Area | Change |
| --- | --- |
| Tokenizer | On seeing `/`, peek the next char. If `/`, skip to end of line. If `*`, skip until `*/`. Otherwise emit `TOKEN_SLASH` as before. The comment-skip path must re-enter the whitespace/comment skip loop so adjacent whitespace or comments are also consumed. |
| Parser | No change. |
| AST | No change. |
| Code Gen | No change. |
| Grammar | No change — comments are lexical, not grammatical. |
| Tests | Add valid tests exercising `//` and `/* ... */`, including multi-line block comments. |

## 3. Ambiguity / grammar / spec issues

- The spec's trailing empty bullet (`- `) is cosmetic only.
- The spec does not explicitly mention unterminated `/* ... */`. We
  treat an unterminated block comment as consumed up to EOF silently,
  matching the minimalist error handling of the rest of the lexer —
  no diagnostic is in scope for this stage.
- The spec does not say `//` comments continue past `\n`, so the `//`
  path stops at the newline (the newline is then consumed by ordinary
  whitespace skipping).

## 4. Implementation plan

1. Tokenizer — extend `lexer_skip_whitespace` into a combined
   whitespace + comment skip loop. When `/` is followed by `/` or
   `*`, consume the comment; otherwise let the existing `/` branch
   produce `TOKEN_SLASH`.
2. Parser / AST / Codegen — no changes.
3. Tests — add valid tests: one with line comments, one with block
   comments, one with multi-line block + inline `//`.
4. Grammar doc — no change (comments are lexical).
5. Commit — single commit for the stage.
