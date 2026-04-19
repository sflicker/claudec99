# Milestone Summary — stage-10-01

Stage 10.1 is complete. The compiler now supports `break` and `continue`
inside `while` and `for` loops. `break` exits the innermost enclosing
loop; `continue` jumps to the next iteration — for `while` this is the
condition re-test, and for `for` this runs the update expression before
re-testing the condition. Nested loops are handled correctly: both
statements always affect only the innermost enclosing loop.

Both statements are rejected at parse time when they appear outside any
loop, producing `error: break outside of loop` or `error: continue
outside of loop`.

Internally, the parser tracks a `loop_depth` counter, and the code
generator maintains a small stack of `(break, continue)` label ids
pushed on loop entry. `while` aliases its continue target to the loop
start. `for` emits a distinct continue label positioned before the
update expression so update runs on `continue`. Both loop kinds emit a
unified `.L_break_<id>` label after the loop body.

Test results: 117/117 pass (97 valid + 12 invalid + 8 print_ast). No
regressions. Six new valid tests and two new invalid tests.

Artifacts:
- docs/kickoffs/stage-10-01-kickoff.md
- docs/milestones/stage-10-01-summary.md
- docs/sessions/stage-10-01-transcript.md
- docs/grammar.md synced
