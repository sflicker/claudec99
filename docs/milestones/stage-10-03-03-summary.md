# Stage-10-03-03 Milestone Summary: Standard Switch Support

Stage 10-03-03 brings `switch` to a standard C shape. The switch body
is now any statement, and `case <integer_literal> :` / `default :`
are `<labeled_statement>`s that may appear anywhere a statement is
legal inside a switch body (including nested blocks and inner
control-flow constructs within the same switch).

Completed in this stage:

- Grammar updated: `<switch_statement> ::= "switch" "(" <expression>
  ")" <statement>`; added `<labeled_statement>` and
  `<constant_expression>`; removed `<switch_body>`,
  `<switch_section>`, `<case_section>`, `<default_section>`; renamed
  `<int_literal>` to `<integer_literal>`.
- Parser: `parse_switch_statement` now parses a single statement
  body. `parse_statement` recognises `case` and `default` labeled
  statements, guarded by `switch_depth > 0`.
- AST: `AST_CASE_SECTION` now holds `[int_literal, inner_statement]`
  and `AST_DEFAULT_SECTION` holds `[inner_statement]`.
- Code generation: a new `SwitchCtx` stack pre-walks the switch body
  (stopping at nested switches) to collect every case/default node
  and assign labels; dispatch is emitted before the body, and the
  body is emitted normally so that unlabeled statements, stacked
  case labels, and fallthrough all work naturally.
- Pretty printer updated to reflect the new AST shape.
- Removed obsolete invalid test `test_invalid_13__expected_token_type.c`
  (it asserted a restriction removed by this stage).
- Added `test_switch_unlabeled_stmt_in_body__42.c` to exercise the
  newly-legal unlabeled statement inside a switch body.
- `docs/grammar.md` synced to the new grammar.
- All 117 valid and 12 invalid tests pass.
