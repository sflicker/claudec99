# Stage-14-00 Milestone: Lexer-Only Mode and Token Tests

## Completed

- Added a `--print-tokens` lexer-only mode to the compiler driver.
  When set, the compiler reads the source, runs the lexer to
  completion, prints one row per token (including the final
  `TOKEN_EOF`), and exits without invoking the parser, AST printer,
  or code generator.
- Output format (all columns left-justified):
  `[N] TOKEN:: <value>  TOKEN_TYPE: <type>` where `N` is the 1-based
  token index padded to the digit count of the largest index in the
  file, `<value>` is the token text in an 18-char field (truncated
  to 15 chars + `...` when longer), and `<type>` is the
  `TokenType` enum identifier.
- New test suite `test/print_tokens/` with `run_tests.sh` patterned
  on `test/print_ast/run_tests.sh`. 46 per-token-type tests (one for
  each meaningful `TokenType`; `TOKEN_EOF` is implicit and
  `TOKEN_UNKNOWN` is intentionally omitted) and 25 program-level
  tests covering decls, control flow, switch, function calls,
  pointers, arrays, increment/decrement, compound assign, logical
  and relational operators, nested blocks, goto/label, break/
  continue, pointer arithmetic, and long-identifier truncation.
- New top-level runner `test/run_all_tests.sh` that runs every
  per-suite runner (valid, invalid, print_ast, print_tokens),
  prints each suite's output, and surfaces an aggregate pass/fail/
  total line. Per the spec, the new suite contributes to the
  project-wide test counts.
- No tokenizer, parser, AST, or codegen change; the stage is
  tooling and tests only.

## Files Changed

- `src/compiler.c` — added `--print-tokens` flag, two-pass lexer
  print loop, and a local `token_type_name()` mapping table.
- `test/print_tokens/` — new directory, 71 `.c`/`.expected` test
  pairs, plus `run_tests.sh`.
- `test/run_all_tests.sh` — new top-level aggregating runner.

## Test Results

- Valid:        232 / 232 pass.
- Invalid:       41 /  41 pass.
- Print-AST:     19 /  19 pass.
- Print-Tokens:  71 /  71 pass (new).
- Aggregate:    363 / 363 pass. No regressions.
