# stage-10-03-02 Kickoff: switch case and dispatch

## Spec Summary

Stage 10-03-02 extends the restricted `switch` statement from stage
10-03-01 to support `case` labels with integer-literal values, in
addition to the optional `default` section. The switch body is now a
sequence of one or more `<switch_section>`s, each of which is a
`<case_section>` (`case <int_literal>:` followed by zero or more
statements) or a `<default_section>` (`default:` followed by zero or
more statements). The controlling expression is evaluated exactly
once, compared against each case value in source order, and control
transfers to the matching section; if no case matches, control
transfers to `default` if present, otherwise to the end of the switch.
`break` exits the switch; `return` behaves normally. Restrictions:
case values must be integer literals, no stacked labels, at most one
`default`, no unlabeled statements outside a section, fallthrough must
not be relied upon in tests, duplicate-case diagnostics are optional.

## Changes from Stage 10-03-01

### Grammar

```ebnf
UPDATE:
<switch_body>    ::= "{" <switch_section> { <switch_section> } "}"

ADD:
<switch_section> ::= <case_section> | <default_section>
<case_section>   ::= "case" <integer_literal> ":" { <statement> }
```

### Tokenizer

- Add `TOKEN_CASE` to `TokenType`.
- Recognize keyword `case` in the identifier branch of
  `lexer_next_token`.

### AST

- Add `AST_CASE_SECTION`. Children: `[IntLiteral, Statement*]` — the
  first child is the case value (AST_INT_LITERAL), followed by the
  section's statements.
- The `AST_SWITCH_STATEMENT` child layout changes from
  `[expression, default_section]` to
  `[expression, section_0, section_1, …]` where each section is
  `AST_CASE_SECTION` or `AST_DEFAULT_SECTION`.
- Extend the pretty printer to render `CaseSection:`.

### Parser

- Replace the "exactly one default" body parse with a loop over
  `<switch_section>` until the closing `}`.
- For each `case`: consume `case`, an integer literal, `:`, then
  statements until the next `case`/`default`/`}`.
- For each `default`: consume `default`, `:`, statements until
  `case`/`default`/`}`; enforce at-most-one `default`.
- Reject any token other than `case`/`default` immediately after `{`
  or between sections (existing `parser_expect` behaviour covers
  this).

### Code Generation

- For each `AST_SWITCH_STATEMENT`:
  1. Evaluate the controlling expression once into `eax`.
  2. Spill `eax` into a fresh stack slot (function frame already has
     space — we will use a push/pop pattern to avoid needing extra
     slot accounting).
  3. For each `case` section, compare the spilled value against the
     case literal and `je` to that section's label.
  4. After all compares, `jmp` to the `default` section's label if a
     default exists, else to the switch-end label.
  5. Emit each section's label followed by its statements.
  6. At the end, emit the switch-end label; `break` jumps there.
- Use the existing `break_stack` frame (`continue_label = -1`).

### Pretty Printer

- Add `CaseSection:` rendering.

### Documentation

- Update `docs/grammar.md` to replace `<switch_body>` /
  `<default_section>` with the new three productions above.

### Tests

- Existing `test_switch_with_case_*.c` tests and
  `test_switch_with_return_inside_case__20.c` cover the new
  functionality and will be exercised by the test runner.
- Run the print_ast and invalid suites and only add tests if gaps are
  found.

## Ambiguities / Decisions

1. **Empty case sections vs. stacked labels.** The spec forbids
   stacked labels (`case 1: case 2: stmt;`), but the grammar
   `<case_section> ::= "case" <integer_literal> ":" { <statement> }`
   allows zero-statement sections, which is equivalent to stacked
   labels at the AST level. The spec also says tests must not rely on
   fallthrough. I will implement the grammar literally: empty
   sections are accepted syntactically, and codegen emits them
   back-to-back (natural C-style fallthrough falls out for free but
   is unexercised by tests). No explicit "stacked label" diagnostic
   is added.
2. **Case value form.** Spec says integer literals only, so the
   parser calls `parser_expect(TOKEN_INT_LITERAL)` — no unary-minus,
   no constant expressions.
3. **Duplicate case diagnostics.** Spec marks this optional. Skipped
   to keep changes minimal.
4. **Switch expression evaluation.** Evaluated exactly once. Codegen
   stores the result on the stack (via `push rax` / `pop rcx`) so
   compares read from a single snapshot.

## Planned File Updates

| File | Change |
|---|---|
| `include/token.h` | Add `TOKEN_CASE` |
| `src/lexer.c` | Recognize `case` |
| `include/ast.h` | Add `AST_CASE_SECTION` |
| `src/ast_pretty_printer.c` | Render `CaseSection:` |
| `src/parser.c` | Parse sequence of switch sections |
| `src/codegen.c` | Emit compare-and-branch dispatch |
| `docs/grammar.md` | Sync switch productions |
| Tests | Run existing suite; add tests only if gaps are found |

## Test Plan

- **Valid (existing):**
  - `test_switch_with_case_1__42.c` — matches first case.
  - `test_switch_with_case_2__72.c` — matches second case.
  - `test_switch_with_case_default__99.c` — no case matches; default runs.
  - `test_switch_with_case_no_default__5.c` — no case matches, no default.
  - `test_switch_with_return_inside_case__20.c` — `return` inside a case.
  - Plus the full stage-10-03-01 switch suite for regression.
- **print_ast:** existing `test_switch.c` (default only) should still match.
- **invalid:** existing `test_invalid_13` (non-section statement in body)
  should still be rejected.

## Next Step

Implement in this order: tokenizer → AST → parser → codegen → pretty
printer → docs → tests → milestone/transcript → commit.
