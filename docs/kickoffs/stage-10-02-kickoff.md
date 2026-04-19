# stage-10-02 Kickoff: do..while

## Spec Summary

Stage 10.2 adds the `do..while` statement to the compiler. It is a loop
construct whose body executes first and whose controlling expression is
evaluated after each iteration. Because the body runs before the first
test, the body is guaranteed to execute at least once. A `do..while`
establishes a new loop context, so `break` and `continue` are valid
inside the body:

- `break` exits the innermost enclosing loop.
- `continue` transfers control to the loop condition check — not to the
  top of the body.

## Changes from Stage 10.1

### Grammar

New production:

```ebnf
<do_while_statement> ::= "do" <statement> "while" "(" <expression> ")" ";"
```

Added as an alternative to `<statement>`. `while` does not need a new
token — it is already handled.

### Tokenizer
- Add `TOKEN_DO` to `TokenType`.
- Recognize the keyword `do` in the identifier branch of
  `lexer_next_token`.

### AST
- Add `AST_DO_WHILE_STATEMENT` (children: body, condition).
- Extend the pretty printer to render `DoWhileStmt`.

### Parser
- Parse `do <statement> while ( <expression> ) ;` as
  `AST_DO_WHILE_STATEMENT`.
- Bump `loop_depth` around the body parse so `break`/`continue` inside
  the body are accepted.

### Code Generation
- Push a `(break, continue)` label pair onto the existing loop stack on
  entry to the do..while.
- Emit body at a loop start label.
- Emit the `.L_continue_<id>:` label immediately before the condition
  check so `continue` jumps to the condition test (per spec).
- Evaluate the condition and jump back to the loop start if non-zero.
- Emit the `.L_break_<id>:` label at loop exit.

### Tests
- Valid tests: executes body at least once when condition is false
  initially; counting loop; `break` inside `do..while`; `continue`
  inside `do..while` skips to condition; nested `do..while`.
- print_ast test covering the new node.

### Documentation
- Update `docs/grammar.md` with the new `<do_while_statement>`
  production and updated `<statement>` production.

## Ambiguities / Decisions

1. **AST child ordering**: body first, condition second — mirrors the
   source order of `do <body> while (<cond>)`.
2. **Continue target placement**: immediately before the condition
   check, per spec language.
3. **Parser loop_depth**: bumped around body parse, same as `while` and
   `for`.
4. **Label naming**: `.L_do_start_<id>`, `.L_continue_<id>`,
   `.L_do_end_<id>`, `.L_break_<id>` to match the existing style.

## Planned File Updates

| File | Change |
|---|---|
| `include/token.h` | Add `TOKEN_DO` |
| `src/lexer.c` | Recognize `do` keyword |
| `include/ast.h` | Add `AST_DO_WHILE_STATEMENT` |
| `src/ast_pretty_printer.c` | Render `DoWhileStmt` |
| `src/parser.c` | Parse do..while; bump/restore `loop_depth` |
| `src/codegen.c` | Emit do..while with loop label stack |
| `docs/grammar.md` | Sync grammar |
| `test/valid/*.c` | New valid tests |
| `test/print_ast/*` | New print_ast test for do..while |

## Test Plan

- **Valid:**
  - `do..while` executes body at least once even when condition is
    initially false.
  - `do..while` as a counting loop reaches 42.
  - `break` inside `do..while` exits the loop.
  - `continue` inside `do..while` jumps to the condition check.
  - Nested `do..while`.
- **print_ast:**
  - Pretty-print of a simple `do..while` statement.

## Next Step

Implement in this order: tokenizer → AST → parser → codegen → tests →
grammar doc → milestone/transcript → commit.
