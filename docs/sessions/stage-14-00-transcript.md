# stage-14-00 Transcript: Lexer-Only Mode and Token Tests

## Summary

This stage adds a `--print-tokens` lexer-only mode to the compiler
and a companion test suite `test/print_tokens/`. The flag runs only
the lexer over the input and prints one row per token, formatted
with left-justified columns: a bracketed 1-based index, the token
text (truncated to 15 chars + `...` when longer than 15), and the
`TokenType` enum name. The driver stops before the parser and
generates no `.asm` output.

The new suite mirrors `test/print_ast/`: per-test `.c`/`.expected`
pairs, `run_tests.sh` that diffs compiler output against expected,
PASS/FAIL per test plus a totals line. Coverage is one test per
meaningful token type plus 25 small representative programs. A new
top-level `test/run_all_tests.sh` aggregates the four suites'
counts so the new tests contribute to the project-wide totals as
required.

No tokenizer, parser, AST, or codegen change was required; the
stage is purely tooling and tests.

## Changes Made

### Step 1: Compiler Driver

- Added `--print-tokens` to the argument parser in `main`.
- Added local `token_type_name()` mapping `TokenType` enum values
  to their identifier strings.
- Added `print_tokens_mode()` that lexes the entire input into a
  growable `Token` buffer, computes the digit width of the final
  token count, then prints one row per token plus a final
  `TOKEN_EOF` row.
- Output format implemented as `[%-*d] TOKEN:: %s  TOKEN_TYPE: %s`
  with the value field built into a fixed 18-char buffer
  (left-justified, truncated to 15 + `...` when longer).
- Updated the usage string to include the new flag.

### Step 2: Tests — print_tokens harness

- Created `test/print_tokens/` with `run_tests.sh` patterned on the
  print_ast runner: iterates `test_*.c`, runs the compiler with
  `--print-tokens`, diffs stdout against `${name}.expected`, prints
  PASS/FAIL plus a totals line.
- Wrote 46 per-token-type tests (`test_token_<type>.c`), one for
  each meaningful `TokenType` value. `TOKEN_EOF` is implicit in
  every test; `TOKEN_UNKNOWN` is omitted.
- Wrote 25 program-level tests (`test_program_<name>.c`) covering
  simple main, var decls, if/else, while, do/while, for, switch,
  function call, function with params, pointer decl, address-of,
  dereference, array decl, array index, increment, decrement,
  compound assign, logical ops, relational ops, arithmetic, nested
  blocks, goto, break/continue, pointer arithmetic, and a
  long-identifier truncation case.
- Generated each `.expected` file via the new compiler mode and
  spot-checked single-token, single-operator, and the truncation
  case to confirm the lexer output matches intent.

### Step 3: Top-level Test Runner

- Added `test/run_all_tests.sh` which invokes every per-suite
  runner (`valid`, `invalid`, `print_ast`, `print_tokens`), echoes
  each suite's full output, parses the `Results: P passed, F
  failed, T total` line, and prints a final aggregate line.
- Returns non-zero if any individual suite returned non-zero.

## Final Results

All four suites green:

- Valid:        232 / 232
- Invalid:       41 /  41
- Print-AST:     19 /  19
- Print-Tokens:  71 /  71  (new)
- **Aggregate:  363 / 363**, no regressions.

## Session Flow

1. Read the stage spec and the supporting `notes.md`,
   `transcript-format.md`, and `stage-kickoff-template.md`.
2. Surveyed the compiler driver, lexer, token enum, and the
   existing `test/print_ast/run_tests.sh` for style and
   conventions.
3. Wrote the kickoff document; flagged seven format-detail
   ambiguities and proposed concrete defaults (column widths,
   `TOKEN_EOF` inclusion, etc.).
4. Confirmed left-justification with the user; proceeded.
5. Modified `src/compiler.c` to add `--print-tokens` and the
   two-pass print loop. Built and ran smoke tests covering single
   token, single operator, long-identifier truncation, and a
   100+ token file.
6. Created `test/print_tokens/` with `run_tests.sh`; wrote 46
   per-type and 25 program-level test inputs; generated each
   `.expected` file via the new compiler mode.
7. Ran the new suite (71/71 pass) and spot-checked alignment and
   truncation in the generated expected files.
8. Added `test/run_all_tests.sh` aggregating runner and confirmed
   all four suites pass with no regressions (363/363).
9. Wrote milestone, transcript, and committed the stage.

## Notes

- `TOKEN_UNKNOWN` is deliberately not exercised by a per-type test;
  it only fires on garbage input and would yield brittle expected
  output. If desired later, a single malformed-input test can be
  added.
- `TOKEN_EOF` is included as the trailing row of every test so the
  `.expected` file also encodes the lexer terminating cleanly.
- No grammar change in this stage — the flag is a tooling concern.
