/implement-stage docs/stages/ClaudeC99-spec-stage-08-01-logical-operators.md

Read 7 files, listed 4 directories (ctrl+o to expand)

● Bash(git log --oneline -5 test/valid/test_if_else_if__42.c)                                                                                                         
⎿  93c78e1 added a breaking test with chained if..else blocks. will be fixed in stage 8.1

● Bash(git diff test/valid/test_if_else_if__42.c)                                                                                                                     
⎿  diff --git a/test/valid/test_if_else_if__42.c b/test/valid/test_if_else_if__42.c                                                                                 
index 3329224..d7e822e 100644                                                                                                                                    
--- a/test/valid/test_if_else_if__42.c                                                                                                                           
… +13 lines (ctrl+o to expand)

● Stage 8.1 Kickoff — Logical Operators && and ||

1. Summary of the spec

- Add two new tokens: && and ||.
- New AST productions: <logical_or_expression> and <logical_and_expression>, inserted in the precedence chain between <assignment_expression> and
  <equality_expression>.
- Precedence (lowest to highest around this band): assignment → || → && → ==/!= → relational → …
- Semantics: integer result (0/1), short-circuit — RHS skipped when result already decided.

2. What must change from the previous stage

- Tokenizer: recognize && → TOKEN_AND_AND; || → TOKEN_OR_OR.
- Parser: insert parse_logical_or (lowest) and parse_logical_and. parse_expression's non-assignment fallthrough must now call parse_logical_or instead of
  parse_equality.
- AST: no new node types — reuse AST_BINARY_OP with value "&&" / "||". Extend operator_name in the pretty-printer for the new operators.
- Codegen: AST_BINARY_OP handler must special-case && and || before the generic left/push/right/pop flow (because short-circuit means we must not emit the RHS      
  unconditionally). Emit compare-and-jump sequences with labels.
- Tests: test_logical_and__1.c and test_logical_or__1.c already exist in test/valid/. Expected exit code is 1 for both (per filename convention). No other test     
  changes expected.

3. Implementation order

1. Tokenizer — add two new token enum values and lexer cases.
2. Parser — add parse_logical_or, parse_logical_and, re-point fallthrough.
3. AST — extend operator_name only.
4. Codegen — emit short-circuit jumps.
5. Verify — run test/valid/run_tests.sh for the two new tests.

4. Observations on the spec

- The grammar is consistent and unambiguous; I have no objections.
- One minor typo: line 80 reads <int_literal> ::= [0-9]+c — trailing c looks like a stray character (present in prior stage specs too, so I'll treat it as a
  harmless holdover, not something to act on).

Ready to proceed with Step 1 (tokenizer)? I'll wait for your confirmation before editing.                                                                 