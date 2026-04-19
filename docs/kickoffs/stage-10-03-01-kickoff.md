# stage-10-03-01 Kickoff: switch and default (restricted)

## Spec Summary

Stage 10-03-01 introduces a restricted `switch` statement. The switch
body must be a braced block containing exactly one `default` label and
no `case` labels (case labels are deferred to a later stage). The
switch expression is evaluated once, control transfers to the
`default` section unconditionally, all statements in the section run
sequentially, and a `break` inside the section exits the switch.
Falling off the end of the section also exits the switch.

## Changes from Stage 10.2

### Grammar

New productions:

```ebnf
<switch_statement> ::= "switch" "(" <expression> ")" <switch_body>
<switch_body>      ::= "{" <default_section> "}"
<default_section>  ::= "default" ":" { <statement> }
```

`<switch_statement>` is added as an alternative of `<statement>`.

### Tokenizer
- Add `TOKEN_SWITCH`, `TOKEN_DEFAULT`, and `TOKEN_COLON` to
  `TokenType`.
- Recognize the keywords `switch` and `default` in the identifier
  branch of `lexer_next_token`.
- Emit `TOKEN_COLON` for the single-character `:`.

### AST
- Add `AST_SWITCH_STATEMENT` (children: expression, default_section).
- Add `AST_DEFAULT_SECTION` (children: zero or more statements).
- Extend the pretty printer to render `SwitchStmt` and
  `DefaultSection`.

### Parser
- Add `parse_switch_statement` for the grammar above.
- Track `switch_depth` in `Parser` so `break` is accepted inside a
  switch even when no enclosing loop is present.
- Loosen the bare-`break` validation to allow either `loop_depth > 0`
  or `switch_depth > 0`; update the error message to reflect
  "loop or switch".

### Code Generation
- Rename the existing loop-label stack to a more general
  `break_stack` (+ `break_depth`), since both loops and switches now
  push break-target frames onto it.
- Loops push `{break_label, continue_label}` as before.
- Switches push `{break_label, -1}` — the `-1` sentinel marks a
  non-loop frame.
- `AST_BREAK_STATEMENT` jumps to the top frame's `break_label`.
- `AST_CONTINUE_STATEMENT` walks down the stack to find the nearest
  frame whose `continue_label != -1` (parser guarantees one exists).
- `AST_SWITCH_STATEMENT` emits: evaluate the expression (for side
  effects), fall through into the default body, then emit
  `.L_switch_end_<id>:` and `.L_break_<id>:` at the exit.

### Tests
- **Valid**: basic default section sets a variable; `break` terminates
  the default section mid-body; fallthrough to end of default section;
  switch inside a `while` loop plus a follow-up switch summing to 42;
  `break` inside a switch inside a `while` with an outer loop `break`;
  `return` inside a default section.
- **Invalid**: non-`default` statement first inside the switch body is
  rejected with an "expected token type" error.
- **print_ast**: pretty-print of a simple switch/default with a break.

### Documentation
- Update `docs/grammar.md` with `<switch_statement>`, `<switch_body>`,
  and `<default_section>`; add `<switch_statement>` to the
  `<statement>` alternatives.

## Ambiguities / Decisions

1. **Spec typo**: "braced" is misspelled "bracd" in the restrictions
   section. Treated as "braced" — implemented the grammar verbatim.
2. **Colon not listed as a token**: the spec's Tokens section lists
   `switch` and `default` but omits `:`. Added `TOKEN_COLON` as the
   obvious necessity to parse `default ":"`.
3. **Default section body shape**: the EBNF is a flat
   `{ <statement> }` following `default :`, not a nested block.
   Implemented that way — no additional `{ ... }` wrapping is
   required or accepted at the default-section level (though a
   statement inside can itself be a block).
4. **Break outside loop error message**: extended to "break outside
   of loop or switch". The existing `test_invalid_11` fragment
   (`break outside of loop`) remains a substring, so the test still
   passes.
5. **Codegen stack rename**: `loop_stack`/`loop_depth` renamed to
   `break_stack`/`break_depth` to reflect that both loops and
   switches push onto it. Continue still targets the nearest loop via
   a sentinel (`continue_label == -1` marks a non-loop frame).

## Planned File Updates

| File | Change |
|---|---|
| `include/token.h` | Add `TOKEN_SWITCH`, `TOKEN_DEFAULT`, `TOKEN_COLON` |
| `src/lexer.c` | Recognize `switch`/`default`; emit `TOKEN_COLON` |
| `include/ast.h` | Add `AST_SWITCH_STATEMENT`, `AST_DEFAULT_SECTION` |
| `src/ast_pretty_printer.c` | Render `SwitchStmt` and `DefaultSection` |
| `include/parser.h` | Add `switch_depth` field |
| `src/parser.c` | Parse switch; allow `break` inside switch |
| `include/codegen.h` | Rename loop stack to break stack |
| `src/codegen.c` | Update loop emitters; emit switch; continue walks down |
| `docs/grammar.md` | Sync grammar |
| `test/valid/*.c` | New valid tests |
| `test/invalid/*.c` | New invalid test |
| `test/print_ast/*` | New print_ast test |

## Test Plan

- **Valid:**
  - Basic switch with default setting a variable.
  - `break` halts default body mid-execution.
  - Fallthrough past end of default body exits the switch.
  - Switch inside a while loop accumulating a sum.
  - `break` inside switch-in-loop exits only the switch.
  - `return` inside a default section.
- **Invalid:**
  - A statement other than `default:` first in the switch body.
- **print_ast:**
  - Pretty-print of switch with a default section containing an
    assignment and a break.

## Next Step

Implement in this order: tokenizer → AST → parser → codegen → tests →
grammar doc → milestone/transcript → commit.
