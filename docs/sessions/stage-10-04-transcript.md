# stage-10-04 Transcript: goto Statement

## Summary

Stage 10-04 adds the C `goto` statement and the identifier form of
`<labeled_statement>` to the Claude C99 compiler. A `goto <identifier>;`
transfers control to the matching `<identifier>:` label within the
enclosing function. Label names must be unique within a function;
duplicate labels and references to undefined labels are rejected at
compile time.

Assembly labels for user labels are emitted as
`.L_usr_<func>_<name>`, so the same label name may be reused across
different functions without colliding in the object file. The
implementation does not support computed goto or label addresses.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_GOTO` to `TokenType` in `include/token.h`.
- Extended the keyword table in `src/lexer.c` so that the identifier
  `goto` produces `TOKEN_GOTO`.

### Step 2: AST

- Added `AST_GOTO_STATEMENT` and `AST_LABEL_STATEMENT` to
  `ASTNodeType` in `include/ast.h`.
- `AST_LABEL_STATEMENT` stores the label name in `value` and keeps the
  labeled statement as its single child.
- `AST_GOTO_STATEMENT` stores the target label name in `value` and has
  no children.

### Step 3: Parser

- At the top of `parse_statement` (in `src/parser.c`) a one-token
  lookahead detects `IDENTIFIER ":"` and parses it as a labeled
  statement, then parses the following statement as its child.
- Added a `TOKEN_GOTO` branch in `parse_statement` that consumes
  `goto <identifier> ;` and returns an `AST_GOTO_STATEMENT`.

### Step 4: Pretty Printer

- Added two cases in `src/ast_pretty_printer.c`: `AST_GOTO_STATEMENT`
  prints `GotoStmt: <name>` and `AST_LABEL_STATEMENT` prints
  `LabelStmt: <name>` before its child statement.

### Step 5: Code Generator

- Added `MAX_USER_LABELS`, `user_labels`, `user_label_count`, and
  `current_func` to `CodeGen` in `include/codegen.h`.
- Added `collect_user_labels` in `src/codegen.c` to recursively walk a
  function body and register every label; duplicates abort with
  `error: duplicate label '...' in function '...'`.
- `codegen_function` resets the per-function label table, sets
  `current_func`, and runs the pre-walk before emitting the body.
- Added `AST_LABEL_STATEMENT` handling that emits
  `.L_usr_<func>_<name>:` and then emits the child statement.
- Added `AST_GOTO_STATEMENT` handling that looks the target up in the
  label table, aborts with `error: undefined label '...' in function
  '...'` if missing, and otherwise emits
  `jmp .L_usr_<func>_<name>`.

### Step 6: Tests

- `test/invalid/test_invalid_13__duplicate_label.c` — same label used
  twice in one function; expects `duplicate label` error.
- `test/invalid/test_invalid_14__undefined_label.c` — `goto` to a
  label that does not exist; expects `undefined label` error.
- `test/print_ast/test_goto.c` + `.expected` — exercises forward and
  backward goto with labeled statements.

### Step 7: Documentation

- `docs/grammar.md` — added the identifier alternative to
  `<labeled_statement>` and the `goto` alternative to
  `<jump_statement>`.

## Final Results

Build: clean.

Test results:
- valid: 121 passed / 0 failed (includes the three provided goto
  tests).
- invalid: 14 passed / 0 failed (includes the two new goto tests).
- print_ast: 10 passed / 1 failed. The single failure is a
  pre-existing `test_switch` expected-file mismatch on `master`; it
  is not caused by this stage and is out of scope.

No regressions introduced by this stage.

## Session Flow

1. Read the stage spec and relevant project files (lexer, parser, AST,
   codegen, pretty printer, existing tests).
2. Produced the Kickoff Summary and saved it to `docs/kickoffs`.
3. Implemented the tokenizer change.
4. Added the two new AST node types and parser support.
5. Updated the pretty printer.
6. Added the per-function user-label table to codegen, wired up the
   pre-walk, and emitted labels / gotos.
7. Built and ran all three test suites; verified no regressions.
8. Added the new invalid and print_ast tests.
9. Updated `docs/grammar.md`.
10. Wrote the milestone summary and this transcript.
