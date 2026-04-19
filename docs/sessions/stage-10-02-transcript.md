# stage-10-02 Transcript: do..while

## Summary

Stage 10.2 adds the `do..while` statement to the compiler. The body
always executes at least once, then the condition is evaluated; if
non-zero, control returns to the top of the body. `do..while`
establishes a loop context, so `break` and `continue` are valid inside
its body. `break` exits the loop; `continue` transfers control to the
condition check, matching the spec.

The tokenizer recognizes the new `do` keyword. The parser adds a
`<do_while_statement>` production and bumps `loop_depth` around the
body so `break`/`continue` are accepted. The code generator reuses the
existing `(break, continue)` label stack and positions the
`.L_continue_<id>:` label immediately before the condition evaluation
so that `continue` jumps there.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_DO` to `TokenType` in `include/token.h`.
- Mapped the keyword `do` to `TOKEN_DO` in `src/lexer.c`.

### Step 2: AST

- Added `AST_DO_WHILE_STATEMENT` to `ASTNodeType` in `include/ast.h`
  (two children: body and condition).
- Extended `ast_pretty_print` in `src/ast_pretty_printer.c` to render
  `DoWhileStmt`.

### Step 3: Parser

- Added `parse_do_while_statement` in `src/parser.c` that parses
  `do <statement> while ( <expression> ) ;`.
- Bumped `loop_depth` before parsing the body and decremented after,
  so `break`/`continue` inside the body are accepted.
- Extended `parse_statement` to dispatch on `TOKEN_DO`.
- Updated the `<statement>` comment to include `<do_while_statement>`.

### Step 4: Code Generator

- Added emission for `AST_DO_WHILE_STATEMENT` in `codegen_statement`:
  - Push `(break, continue)` label ids onto the loop stack.
  - Emit `.L_do_start_<id>:` and the body.
  - Emit `.L_continue_<id>:` (target for `continue`).
  - Evaluate the condition; `jne .L_do_start_<id>` to loop back.
  - Emit `.L_do_end_<id>:` and `.L_break_<id>:` (target for `break`).

### Step 5: Tests

- Added five valid tests under `test/valid/`:
  `test_do_while_at_least_once__1.c`,
  `test_do_while_countup__42.c`,
  `test_break_in_do_while__42.c`,
  `test_continue_in_do_while__42.c`,
  `test_do_while_nested__42.c`.
- Added one new AST pretty-print test under `test/print_ast/`:
  `test_do_while.c` + `test_do_while.expected`.

### Step 6: Documentation

- Updated `docs/grammar.md` with the `<do_while_statement>` production
  and listed it under `<statement>`.

## Final Results

All 123 tests pass (102 valid + 12 invalid + 9 print_ast). No
regressions. Five new valid tests and one new print_ast test added
this stage.

## Session Flow

1. Read spec `ClaudeC99-spec-stage-10-02-do-while.md` and supporting
   skill files.
2. Reviewed existing tokenizer, parser, AST, and codegen for loop and
   jump-statement handling from stage 10.1.
3. Produced kickoff summary and implementation plan.
4. Implemented tokenizer keyword `do`.
5. Added AST node and pretty-printer case for `DoWhileStmt`.
6. Added parser production `parse_do_while_statement` with `loop_depth`
   tracking.
7. Extended code generator to emit `do..while` with continue target at
   the condition check.
8. Added five valid tests and one print_ast test.
9. Built the compiler and ran all test suites — 123/123 pass.
10. Updated `docs/grammar.md` and wrote kickoff, milestone, and
    transcript artifacts.

## Tests

| Test | Kind | Expectation |
|---|---|---|
| `test_do_while_at_least_once__1.c` | valid | body runs once even when condition initially false |
| `test_do_while_countup__42.c` | valid | counts to 42 via do..while |
| `test_break_in_do_while__42.c` | valid | `break` exits the do..while when `a == 42` |
| `test_continue_in_do_while__42.c` | valid | `continue` jumps to condition check; sum omits i=3 |
| `test_do_while_nested__42.c` | valid | nested do..while: 6 × 7 = 42 |
| `test_do_while` (print_ast) | valid | AST pretty-prints `DoWhileStmt` with body then condition |

## Notes

- AST child ordering mirrors source order: `children[0]` = body,
  `children[1]` = condition.
- Continue label placement: emitted *before* the condition check, so
  `continue` jumps directly to re-evaluation — matching the spec.
- Label naming style follows stage 10.1: `.L_do_start_<id>`,
  `.L_continue_<id>`, `.L_do_end_<id>`, `.L_break_<id>`.
- `parse_statement` already enforces "break/continue outside loop" via
  `loop_depth`; bumping it around the do..while body preserves that
  check without any new invalid tests needed.
