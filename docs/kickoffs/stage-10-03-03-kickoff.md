# Stage-10-03-03 Kickoff: Standard Switch Support

## 1. Summary of the spec

Stage 10-03-03 moves `switch` from the restricted stage 10-03-02 form
(`switch (...) { case ...: {stmts}  default: {stmts} }`) to a
standard C-shape where the switch body is any statement and
`case <integer_literal> :` / `default :` become `<labeled_statement>`s
— regular statements that may appear within the body (typically a
block).

Behavior under the new grammar:

- The controlling expression is evaluated exactly once.
- Control transfers to the first matching `case` label; if none
  matches, to `default` if present, otherwise past the switch.
- After entering a labeled statement, execution continues
  sequentially through subsequent statements until `break`, `return`,
  or the end of the switch (fallthrough).
- Stacked `case` labels may apply to a single statement.
- Unlabeled statements may appear anywhere in the switch body
  (reachable only via fallthrough, never by dispatch).
- `case` expressions remain restricted to `<integer_literal>`;
  `<int_literal>` is renamed to `<integer_literal>` in the grammar.

## 2. What must change from the previous stage

| Area | Change |
| --- | --- |
| Grammar | Remove `<switch_body>`, `<switch_section>`, `<case_section>`, `<default_section>`. Redefine `<switch_statement> ::= "switch" "(" <expression> ")" <statement>`. Add `<labeled_statement> ::= "case" <constant_expression> ":" <statement> | "default" ":" <statement>`. Add `<constant_expression> ::= <integer_literal>`. Rename `<int_literal>` → `<integer_literal>`. |
| Tokenizer | No changes. |
| Parser | `parse_switch_statement` becomes `switch (expr) <statement>`. Top-level `parse_statement` handles `case`/`default` as labeled statements guarded by `switch_depth > 0`. |
| AST | Keep names `AST_CASE_SECTION` / `AST_DEFAULT_SECTION`. Shape changes: case node = `[int_literal, inner_statement]`, default node = `[inner_statement]`. |
| Code generation | Pre-walk switch body (not descending into nested switches) to collect case/default node pointers and assign labels. Emit dispatch before the body. Emit the body as ordinary statements; when a case/default node is visited, output its pre-assigned label. Keep the switch-end + break label. |
| Pretty printer | Adjust the two cases for the new shape. |
| Tests | Existing switch tests must still pass. Add a test exercising an unlabeled statement in a switch body. |
| Docs | Update `docs/grammar.md`. |

## 3. Ambiguity / grammar / spec issues

- `<int_literal>` → `<integer_literal>` is a documentation-level
  rename; internal token/AST names (`TOKEN_INT_LITERAL`,
  `AST_INT_LITERAL`) can remain, per the spec's allowance for
  internal representation differences.
- `<constant_expression> ::= <integer_literal>` — no new AST node
  needed; case labels continue to parse a single `TOKEN_INT_LITERAL`.
- The new grammar naturally allows labels nested inside blocks or
  other control-flow statements within the same switch. The pre-walk
  descends through inner statements and stops at nested `switch`
  bodies.

## 4. Implementation plan

1. Parser — rewrite `parse_switch_statement`; add `case` / `default`
   handling in `parse_statement`, guarded by `switch_depth > 0`.
2. AST — no struct changes; update the shape of `AST_CASE_SECTION` /
   `AST_DEFAULT_SECTION` to hold a single inner statement.
3. Pretty printer — update those two cases.
4. Code generation — add `SwitchCtx` stack to `CodeGen`; pre-walk
   label collection; rework `AST_SWITCH_STATEMENT`; handle
   `AST_CASE_SECTION` / `AST_DEFAULT_SECTION` during statement
   emission.
5. Tests — add a positive test for unlabeled statements in the
   switch body; keep the 116 existing tests green.
6. Docs — update `docs/grammar.md`.
7. Commit — single commit for the stage.
