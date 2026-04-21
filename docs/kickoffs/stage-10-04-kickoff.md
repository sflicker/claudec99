# Stage-10-04 Kickoff: goto Statement

## 1. Summary of the spec

Stage 10-04 adds C's `goto` statement and the identifier form of
`<labeled_statement>`. A `goto <identifier>;` transfers control to the
matching `<identifier>:` label within the same function.

- Labels must be unique within their function (duplicate = compile error).
- `goto` to a missing label is a compile error.
- `goto` cannot jump out of its function.
- No computed goto. No label addresses.
- Assembly label names must be unique across the whole output, not just
  unique per-function (handled by per-function prefixing in codegen).

## 2. What must change from the previous stage

| Area | Change |
| --- | --- |
| Grammar | Add identifier form to `<labeled_statement>`. Add `"goto" <identifier> ";"` alternative to `<jump_statement>`. |
| Tokenizer | Add `TOKEN_GOTO`; recognize the `goto` keyword. |
| AST | Add `AST_GOTO_STATEMENT` (value=target label) and `AST_LABEL_STATEMENT` (value=label name, one child = the labeled statement). |
| Parser | At the top of `parse_statement` use a one-token lookahead: if an `IDENTIFIER` is followed by `:` treat it as a labeled statement. Add a branch for `goto <identifier> ;`. |
| Code gen | Per-function label table populated by a pre-walk of the function body; duplicate labels rejected; missing `goto` targets rejected. Emit labels as `.L_usr_<func>_<name>` so names are unique across the module. |
| Pretty printer | Add `GotoStmt: <name>` and `LabelStmt: <name>` cases. |
| Tests | Three valid tests already provided. Add invalid tests for duplicate/undefined labels and a print_ast test. |
| Docs | Update `docs/grammar.md`. |

## 3. Ambiguity / grammar / spec issues

- The spec's EBNF has `<statement` (no closing `>`) in the first
  `<labeled_statement>` alternative — obvious typo.
- The spec says label names must be unique within the same function, so
  different functions may reuse identical label names. Codegen must
  avoid assembly-level collisions; per-function prefixing handles it.
- The spec adds only the `<identifier>` form to `<labeled_statement>`
  in this stage, and `<jump_statement>` only takes `<identifier>` as
  `goto` target. `goto` therefore targets only identifier labels, not
  `case`/`default` labels — matching standard C.
- A label statement labels the statement that follows. Label statements
  may appear anywhere in the function body (including inside loops,
  `if`s, blocks). The label-collection pre-walk recurses through the
  body.

## 4. Implementation plan

1. Tokenizer — add `TOKEN_GOTO` and recognize the keyword.
2. AST — add `AST_GOTO_STATEMENT` and `AST_LABEL_STATEMENT` node types.
3. Parser — add label-statement lookahead at the top of
   `parse_statement`; add a `goto <identifier> ;` branch.
4. Pretty printer — add the two new node cases.
5. Code generation — add a `GotoCtx` (user-label table) to `CodeGen`,
   run a pre-walk for each function to collect labels (reject
   duplicates), emit `.L_usr_<func>_<name>` for labels and gotos, and
   reject unknown `goto` targets during emission.
6. Tests — run existing tests; add invalid tests for duplicate and
   undefined labels, and a print_ast test.
7. Docs — update `docs/grammar.md`.
8. Commit — single commit for the stage.
