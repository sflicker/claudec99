# stage-10-01 Transcript: break and continue

## Summary

Stage 10.1 adds the `break` and `continue` jump statements to the
compiler. `break` exits the innermost enclosing `while` or `for` loop.
`continue` proceeds to the next iteration of the innermost enclosing
loop — for `while` this means re-evaluate the condition, and for `for`
this means evaluate the update expression and then re-evaluate the
condition. Both statements affect only the innermost loop and are
parse-time errors when used outside any loop.

The tokenizer recognizes two new keywords. The parser tracks a
`loop_depth` counter to enforce the "inside a loop" rule. The code
generator maintains a stack of loop label ids so that nested loops
resolve their break/continue targets correctly; the `for` loop emits a
dedicated continue label so that `continue` still runs the update.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_BREAK` and `TOKEN_CONTINUE` to `TokenType` in
  `include/token.h`.
- Mapped keywords `break` and `continue` to the new tokens in
  `src/lexer.c`.

### Step 2: AST

- Added `AST_BREAK_STATEMENT` and `AST_CONTINUE_STATEMENT` to
  `ASTNodeType` in `include/ast.h` (leaf nodes with no children).
- Extended `ast_pretty_print` in `src/ast_pretty_printer.c` to render
  `BreakStmt` and `ContinueStmt`.

### Step 3: Parser

- Added `loop_depth` field to `Parser` in `include/parser.h` and
  initialized it in `parser_init`.
- Bumped `loop_depth` before parsing the body of `parse_while_statement`
  and `parse_for_statement`; decremented on return.
- Extended `parse_statement` in `src/parser.c` to parse
  `TOKEN_BREAK ";"` and `TOKEN_CONTINUE ";"` into the new AST nodes.
- Emitted `error: break outside of loop` /
  `error: continue outside of loop` when `loop_depth == 0`.

### Step 4: Code Generator

- Added `LoopLabels` (break/continue label ids) plus a 32-deep
  `loop_stack` and `loop_depth` field to `CodeGen` in
  `include/codegen.h`.
- In `codegen_statement` for `AST_WHILE_STATEMENT`, pushed the loop's
  label id as both break and continue target; emitted an aliased
  `.L_continue_<id>:` before the condition test and `.L_break_<id>:`
  after `.L_while_end_<id>:`.
- In `codegen_statement` for `AST_FOR_STATEMENT`, emitted
  `.L_continue_<id>:` between the body and the update expression, and
  `.L_break_<id>:` after `.L_for_end_<id>:`.
- Added emission for `AST_BREAK_STATEMENT` and
  `AST_CONTINUE_STATEMENT`: `jmp` to the top-of-stack break / continue
  label.

### Step 5: Tests

- Added six valid tests under `test/valid/`:
  `test_break_in_while__42.c`, `test_break_in_for__42.c`,
  `test_continue_in_while__42.c`, `test_continue_in_for__42.c`,
  `test_break_nested__42.c`, `test_continue_nested__42.c`.
- Added two invalid tests under `test/invalid/`:
  `test_invalid_11__break_outside_of_loop.c`,
  `test_invalid_12__continue_outside_of_loop.c`.

### Step 6: Documentation

- Updated `docs/grammar.md` to include the `<jump_statement>`
  production and list it in `<statement>`.

## Final Results

All 117 tests pass (97 valid + 12 invalid + 8 print_ast). No
regressions. Six new valid tests and two new invalid tests added this
stage.

## Session Flow

1. Read spec `ClaudeC99-spec-stage-10-01-break-and-continue.md` and
   supporting skill files.
2. Reviewed existing tokenizer, parser, AST, and codegen for loop
   handling.
3. Produced kickoff summary and implementation plan.
4. Implemented tokenizer keywords.
5. Added AST nodes and pretty-printer cases.
6. Added parser loop-depth tracking and jump-statement parsing.
7. Extended code generator with loop label stack and emission for
   break/continue.
8. Added six valid and two invalid tests; tuned error messages to match
   the invalid-test filename-fragment convention.
9. Built the compiler and ran all test suites — 117/117 pass.
10. Updated grammar.md and wrote kickoff, milestone, and transcript
    artifacts.

## Tests

| Test | Kind | Expectation |
|---|---|---|
| `test_break_in_while__42.c` | valid | `break` exits `while` when `i == 42` |
| `test_break_in_for__42.c` | valid | `break` exits `for` when `i == 42` |
| `test_continue_in_while__42.c` | valid | `continue` in `while` skips body; sum omits i=3 |
| `test_continue_in_for__42.c` | valid | `continue` in `for` runs update; sum omits i=3 |
| `test_break_nested__42.c` | valid | inner `break` only exits inner loop (6 × 7 = 42) |
| `test_continue_nested__42.c` | valid | inner `continue` only skips inner body (6 × 7 = 42) |
| `test_invalid_11__break_outside_of_loop.c` | invalid | parser rejects `break` outside loop |
| `test_invalid_12__continue_outside_of_loop.c` | invalid | parser rejects `continue` outside loop |

## Notes

- Break and continue are rejected at parse time via a `loop_depth`
  counter on `Parser` rather than at codegen time. This matches the
  location of other static diagnostics.
- Label naming: `.L_break_<id>` and `.L_continue_<id>` are the unified
  targets used by the emitted jumps; the pre-existing
  `.L_while_start_<id>`, `.L_while_end_<id>`, `.L_for_start_<id>`, and
  `.L_for_end_<id>` labels are retained for internal loop control-flow.
- For `while`, the continue target is aliased to the loop start (both
  labels sit at the same address).
- For `for`, the continue target sits between the body and the update
  so `continue` still runs the update before the next condition test.
