# Stage-14-07-01 Kickoff

## Spec
`docs/stages/Claude-spec-stage-14-07-01-libc-test-adjustment.md`

## Stage Goal
Stage 14-07 added two valid tests that exercise libc `puts`
(`test_libc_puts_hello__0.c` and `test_libc_puts_two_calls__0.c`),
but the `valid` runner only checks exit codes — it never compares
the program's stdout against an expected value. This sub-stage
extends the runner so that, for any test `<name>.c` with a sibling
`<name>.expected` file, the runtime stdout is also compared against
the expected file's contents. Tests without a `.expected` file
continue to be validated by exit code only. Existing tests that
already produce output get back-filled with `.expected` files.

## Delta from Stage 14-07
1. `test/valid/run_tests.sh` — capture each binary's stdout
   (instead of redirecting it to `/dev/null`) and, when a
   `.expected` file exists alongside the source, compare the
   captured stdout against it.
2. `test/valid/run_test.sh` — single-test driver gets the same
   optional comparison.
3. New `.expected` files for the two existing puts tests.

## Ambiguities Flagged
- **Trailing newline / exact-match.** The spec says "compare the
  output … with the values in the .expected file" but doesn't
  specify whether the comparison is byte-exact or
  whitespace-tolerant. The conventional and most useful behavior
  is byte-exact, matching how `print_asm` already does it.
  Implementation goes with byte-exact comparison.
- **Sentence fragment in Requirements bullet 1.** "If there is a
  matching file with the same base name but with the extension
  `.expected`." has no main clause — read in context with bullet 2
  it clearly means "if such a file exists, compare against it."
- **What counts as "back-filling existing tests that are
  generating output"?** The only valid tests known to print are
  the two puts tests added in stage 14-07. Both get `.expected`
  files (`hello\n` and `A\nB\n` respectively, since `puts` appends
  a newline).
- The spec asks the runner only to do the *optional* extra check;
  exit-code checking remains as-is.

## Planned Changes

### Tokenizer (`src/lexer.c`)
- No changes.

### Parser (`src/parser.c`)
- No changes.

### AST
- No changes.

### Code Generator (`src/codegen.c`)
- No changes.

### Test Runner
- `test/valid/run_tests.sh`: capture each binary's stdout into a
  temp file (instead of `>/dev/null`); after the exit-code check,
  if `<name>.expected` exists in the source dir, compare the
  captured stdout to it byte-for-byte; report
  `FAIL (output mismatch)` on diff.
- `test/valid/run_test.sh`: same optional comparison for the
  single-test driver.

### Tests / fixtures
- `test/valid/test_libc_puts_hello__0.expected` — `hello\n`.
- `test/valid/test_libc_puts_two_calls__0.expected` — `A\nB\n`.

### Documentation
- `README.md`: update the `valid` row in the suites table to
  mention the optional stdout check.
- `docs/grammar.md`: no changes.

### Commit
Single commit at the end of the stage.
