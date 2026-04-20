# stage-10-03-02 Transcript: Case Labels and Dispatch

## Summary

Stage 10-03-02 extends the restricted `switch` statement from
stage 10-03-01 to accept `case <integer_literal>:` sections alongside
an optional `default:` section. The switch body is now parsed as a
sequence of sections, each terminated by the next section header or
the closing brace. The controlling expression is evaluated exactly
once and spilled on the stack; a linear compare-and-branch sequence
dispatches to the matching case. If no case matches, control
transfers to the default section when present, otherwise to the end
of the switch. `break` exits the switch; `return` exits the
function.

The stage keeps the restrictions from the spec: case values must be
integer literals, at most one `default` is allowed, stacked labels
are not supported as a feature, and tests do not exercise fallthrough.
Duplicate-case diagnostics and jump-table optimization remain out of
scope.

## Plan

Implementation order: tokenizer → AST → pretty printer → parser →
code generator → docs → tests → milestone/transcript → commit.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_CASE` to `TokenType` in `include/token.h`.
- Recognized the keyword `case` in the identifier branch of
  `lexer_next_token` in `src/lexer.c`.

### Step 2: AST

- Added `AST_CASE_SECTION` to `ASTNodeType` in `include/ast.h`.
- AST layout: first child is the case value (`AST_INT_LITERAL`),
  remaining children are the section's statements.
- Changed the `AST_SWITCH_STATEMENT` layout from
  `[expression, default_section]` to
  `[expression, section_0, section_1, …]`.

### Step 3: Pretty Printer

- Added a `CaseSection: <value>` branch to `ast_pretty_print` in
  `src/ast_pretty_printer.c` that prints the case value and
  recursively prints the section's statement children.

### Step 4: Parser

- Rewrote `parse_switch_statement` in `src/parser.c` to loop over
  `<switch_section>`s until the closing brace.
- Parses `case <integer_literal> ":"` followed by statements until
  the next `case`, `default`, or `}`.
- Parses `default ":"` with the same statement-collection logic.
- Rejects a second `default` with an explicit "duplicate default
  section in switch" error.
- Rejects a body that starts with anything other than `case` or
  `default` by deferring to `parser_expect(TOKEN_CASE)`, which
  yields the existing "expected token type" diagnostic.

### Step 5: Code Generator

- Rewrote `AST_SWITCH_STATEMENT` in `src/codegen.c`:
  - Allocates one label id for the switch and one per section.
  - Evaluates the controlling expression once and `push rax` to
    spill the value.
  - Emits a `mov eax, [rsp]` / `cmp` / `je .L_switch_sec_<n>`
    sequence for each case.
  - Emits a final `jmp` to either the default section's label or
    the switch-end label when no case matches.
  - Pushes a break frame with `continue_label = -1` and emits each
    section's label followed by its statements (skipping the
    integer-literal child for case sections).
  - Converges at `.L_switch_end_<id>:` and `.L_break_<id>:`,
    followed by `add rsp, 8` to pop the spilled value.

```asm
    mov eax, [rbp - 4]      ; evaluate controlling expression
    push rax                ; spill
    mov eax, [rsp]
    cmp eax, 1
    je .L_switch_sec_1
    mov eax, [rsp]
    cmp eax, 2
    je .L_switch_sec_2
    jmp .L_switch_sec_3     ; default
.L_switch_sec_1:
    ...
    jmp .L_break_0
.L_switch_end_0:
.L_break_0:
    add rsp, 8
```

### Step 6: Documentation

- Updated `docs/grammar.md` to replace the old
  `<switch_body> ::= "{" <default_section> "}"` with the new
  `<switch_body>`, `<switch_section>`, and `<case_section>`
  productions.

### Step 7: Tests

- No new tests were authored in this stage — valid tests for case
  matching, default fallback, missing default, and return-in-case
  were already present in `test/valid/` and now pass.

## Final Results

All 136 tests pass (113 valid + 13 invalid + 10 print_ast). No
regressions. Five previously-authored case tests
(`test_switch_with_case_1__42.c`, `test_switch_with_case_2__72.c`,
`test_switch_with_case_default__99.c`,
`test_switch_with_case_no_default__5.c`,
`test_switch_with_return_inside_case__20.c`) now compile and run
green.

## Session Flow

1. Read spec, skill notes, and prior stage artifacts.
2. Reviewed existing tokenizer, AST, parser, codegen, and pretty
   printer to locate the stage-10-03-01 switch implementation.
3. Produced a kickoff summary and saved it to the kickoffs directory.
4. Implemented changes in order: tokenizer, AST, pretty printer,
   parser, codegen, grammar doc.
5. Built the compiler and ran the full test suite.
6. Inspected emitted assembly for a representative case test.
7. Wrote the milestone summary and this transcript.

## Notes

- Empty case sections are syntactically accepted by the grammar. Per
  the spec, stacked labels are not a supported feature and tests must
  not rely on fallthrough, so no special diagnostic is added.
- Switch expression is spilled via `push rax` rather than a dedicated
  stack slot; `cg->push_depth` is bumped so function-call alignment
  inside sections remains correct. `add rsp, 8` is emitted at the
  end label to reclaim the slot.
