# Stage-14-00 Kickoff: Lexer-Only Mode and Token Tests

## Spec Summary

Stage 14-00 adds a `--print-tokens` lexer-only mode to the compiler and
a new `test/print_tokens/` test suite that exercises the existing
tokenizer end-to-end. No grammar, parser, AST, or codegen change is
required.

CLI behavior:
  - `--print-tokens <file>`: run the lexer over the entire input and
    print one line per token. No parse, no AST, no codegen, no `.asm`
    output. Exit 0 on success.
  - Output format per token: `[N] TOKEN:: <value>  TOKEN_TYPE: <type>`
    with all columns left-justified.

Test suite layout, mirroring `test/print_ast/`:
  - `test_<name>.c` paired with `test_<name>.expected`.
  - `run_tests.sh` runs each `.c` through `--print-tokens`, diffs
    stdout against `.expected`, prints PASS/FAIL plus totals.
  - One test per (meaningful) token type plus 20-25 representative
    small programs.
  - Suite contributes to the project-wide test counts via a new
    `test/run_all_tests.sh` that aggregates all four suites.

## What Must Change vs. Stage 13-05

  - `src/compiler.c`: add `--print-tokens` flag and a lexer-only
    print loop. The driver currently understands only `--print-ast`.
  - New `test/print_tokens/` directory with runner, per-type tests,
    and program-level tests.
  - New `test/run_all_tests.sh` so the new suite contributes to the
    project-wide totals (the repo currently has no top-level runner).

No tokenizer, parser, AST, or codegen change.

## Output Format Decisions

  - **Index column**: `[N]` where `N` is the 1-based token index,
    left-justified to a width equal to the digit count of the
    **largest** index in this file. So a file with 100 tokens prints
    `[1  ]`, `[10 ]`, `[100]`. The implementation lexes into a buffer
    first to determine the count, then prints.
  - **TOKEN:: literal**: printed verbatim with two spaces after the
    `::` so the value column has a clear left edge.
  - **Value column**: left-justified to 18 chars. The body holds up
    to 15 chars; if the original token text is longer than 15 it is
    truncated to 15 and `...` is appended (total 18 chars). Shorter
    values are space-padded on the right.
  - **TOKEN_TYPE: literal**: printed verbatim, single space after.
  - **Type column**: the `TokenType` enum identifier, e.g.
    `TOKEN_INT_LITERAL`, `TOKEN_PLUS`, `TOKEN_EOF`. No trailing
    padding.
  - **TOKEN_EOF**: printed as the final row so the harness can
    confirm the lexer terminated cleanly.

Example rendering for `int x = 42;`:

```
[1] TOKEN:: int                TOKEN_TYPE: TOKEN_INT
[2] TOKEN:: x                  TOKEN_TYPE: TOKEN_IDENTIFIER
[3] TOKEN:: =                  TOKEN_TYPE: TOKEN_ASSIGN
[4] TOKEN:: 42                 TOKEN_TYPE: TOKEN_INT_LITERAL
[5] TOKEN:: ;                  TOKEN_TYPE: TOKEN_SEMICOLON
[6] TOKEN::                    TOKEN_TYPE: TOKEN_EOF
```

## Spec Issues Flagged

  1. **Bracket literals.** Spec writes `[###]` as a placeholder for
     the digit field. Implemented as literal `[` and `]` around a
     left-justified, file-max-width digit field. No zero padding.
  2. **`<value>` truncation.** Spec: "Only the first 15 character to
     be display. If the token is longer truncate at 15 and add an
     ...". Implemented as: if `len(value) > 15`, print
     `value[0:15] + "..."`; otherwise print `value` and pad to 18
     with spaces. The 18-char column is the smallest width that
     keeps both forms aligned.
  3. **TOKEN_EOF row.** Spec is silent. Implemented as included.
     Acts as a self-check that the lexer reached end-of-file.
  4. **TOKEN_UNKNOWN per-type test.** Spec says "one test for each
     type" but `TOKEN_UNKNOWN` only fires on garbage input and would
     produce brittle expected output. Skipped from the per-type
     coverage; if the user wants it, a deliberately malformed test
     file can be added later.
  5. **Project-wide test integration.** No top-level test runner
     exists in the repo today; spec requires the new suite to
     contribute to the project totals. Implemented as
     `test/run_all_tests.sh` that runs the four suite scripts and
     prints aggregate counts.

## Planned Changes

  1. **Tokenizer** — none.
  2. **Parser** — none.
  3. **AST** — none.
  4. **Code generator** — none.
  5. **Compiler driver (`src/compiler.c`)** — parse `--print-tokens`,
     add a lexer-only print loop with two passes over an in-memory
     token buffer (first pass counts; second pass prints). Type-name
     mapping table local to the driver.
  6. **Tests** — new `test/print_tokens/` directory:
     - `run_tests.sh` modeled on `test/print_ast/run_tests.sh`.
     - One test per meaningful `TokenType` (~37 tests).
     - 20-25 small programs covering current features (decls, if,
       while, do/while, for, switch, function call, pointer/array
       basics, increment/decrement, compound assign, logical ops).
  7. **Top-level runner (`test/run_all_tests.sh`)** — runs the four
     suite scripts in order, sums totals, exits non-zero on any
     failure.
  8. **Grammar (`docs/grammar.md`)** — no change. Tooling-only stage.
  9. **Docs** — kickoff (this), milestone, session transcript.
 10. **Commit** — single commit when all suites are green.
