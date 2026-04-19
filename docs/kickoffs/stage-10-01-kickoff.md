# stage-10-01 Kickoff: break and continue

## Spec Summary

Stage 10.1 adds two new jump statements — `break` and `continue` — to the
compiler. `break` exits the innermost enclosing loop immediately.
`continue` skips the rest of the current loop body and begins the next
iteration of the innermost enclosing loop. For `while`, "next iteration"
means jump to the condition test. For `for`, "next iteration" means
evaluate the update expression, then the condition test.

Both are valid only inside a `for` or `while` loop. Using either outside
a loop is a compile-time error. Only the two existing loop forms need
support; there is no `do-while` in the language.

## Changes from Stage 9.5

### Grammar

New production:

```ebnf
<jump_statement> ::= "continue" ";"
                   | "break" ";"
```

`<jump_statement>` is added as an alternative to `<statement>`. No other
grammar rules change.

### Tokenizer
- Add `TOKEN_BREAK` and `TOKEN_CONTINUE` to `TokenType`.
- Recognize the keywords `break` and `continue` in the identifier branch
  of `lexer_next_token`.

### AST
- Add `AST_BREAK_STATEMENT` and `AST_CONTINUE_STATEMENT` leaf node kinds.
- Extend the pretty printer to render both.

### Parser
- Track loop-nesting depth with a `loop_depth` counter on `Parser`,
  incremented before parsing the body of `while`/`for` and decremented
  after.
- Parse `break ;` and `continue ;` as their respective AST nodes inside
  `parse_statement`.
- Reject the jump at parse time if `loop_depth == 0`.

### Code Generation
- Maintain a small stack of `(break_label, continue_label)` pairs inside
  `CodeGen`, pushed when emitting a loop and popped on exit.
- Emit the continue target as a distinct label:
  - `while`: `continue` jumps to the condition-test label (same as the
    loop's start label).
  - `for`: `continue` jumps to a label placed *before* the update expression
    so update runs, then control falls into the condition test.
- `break` jumps to the existing end label of the enclosing loop.

### Tests
- 6 valid tests: break/continue in while, break/continue in for, nested
  break (affects innermost only), nested continue (affects innermost only).
- 2 invalid tests: `break` outside loop, `continue` outside loop.

### Documentation
- Update `docs/grammar.md` with the new `<jump_statement>` production.

## Semantic Rules (from spec)

- `break` exits the innermost enclosing `for`/`while` loop.
- `continue` transfers control to the next iteration of the innermost
  enclosing `for`/`while` loop:
  - `for`: continue → update → condition.
  - `while`: continue → condition.
- Either outside a loop is a compile-time error.

## Ambiguities / Decisions

1. **Which phase rejects `break`/`continue` outside a loop.** Parser, at
   parse time, via a `loop_depth` counter. Earlier rejection keeps codegen
   simple and matches how static errors are surfaced elsewhere.
2. **Error message wording.** `"error: 'break' outside of loop"` and
   `"error: 'continue' outside of loop"` — wording is chosen to match
   the filename-fragment convention used by `test/invalid/run_tests.sh`.
3. **`for`-loop continue semantics.** Spec explicitly requires the
   update to run on `continue`. Current `for` codegen runs the update at
   the bottom of the body, then jumps back to start. To support
   `continue`, emit a dedicated continue-label *before* the update so
   that `continue` skips only the remainder of the body and still runs
   the update.
4. **Nested loops.** Each loop pushes a fresh `(break, continue)` pair
   onto a codegen-local stack. `break`/`continue` always read the top
   entry.
5. **AST shape.** Two distinct node kinds (`AST_BREAK_STATEMENT`,
   `AST_CONTINUE_STATEMENT`) rather than a single `AST_JUMP_STATEMENT`,
   matching the style of `AST_IF_STATEMENT`, `AST_WHILE_STATEMENT`.

## Planned File Updates

| File | Change |
|---|---|
| `include/token.h` | Add `TOKEN_BREAK`, `TOKEN_CONTINUE` |
| `src/lexer.c` | Recognize `break`, `continue` keywords |
| `include/ast.h` | Add `AST_BREAK_STATEMENT`, `AST_CONTINUE_STATEMENT` |
| `src/ast_pretty_printer.c` | Print new node kinds |
| `include/parser.h` | Add `loop_depth` to `Parser` |
| `src/parser.c` | Parse jump statement; bump/restore `loop_depth` in while/for |
| `include/codegen.h` | Add break/continue label stack |
| `src/codegen.c` | Push labels in while/for; emit continue target; emit jumps |
| `docs/grammar.md` | Sync grammar |
| `test/valid/*.c` | 6 new valid tests |
| `test/invalid/*.c` | 2 new invalid tests |

## Test Plan

- **Valid:**
  - `break` exits a `while`.
  - `break` exits a `for`.
  - `continue` in `while` skips remaining body and re-evaluates condition.
  - `continue` in `for` skips remaining body and runs update.
  - Nested loops: `break` affects only the innermost loop.
  - Nested loops: `continue` affects only the innermost loop.
- **Invalid:**
  - `break` outside any loop — rejected at compile time.
  - `continue` outside any loop — rejected at compile time.

## Next Step

Implement in this order: tokenizer → AST → parser → codegen → tests →
grammar doc → milestone/transcript → commit.
