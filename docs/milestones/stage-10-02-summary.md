# Milestone Summary — stage-10-02

Stage 10.2 is complete. The compiler now supports the `do..while`
statement. Its body executes before the condition is evaluated, so the
body is guaranteed to run at least once. `break` inside the body exits
the loop, and `continue` transfers control to the condition check (not
to the top of the body), establishing a loop context consistent with
`while` and `for`.

Internally, a new `TOKEN_DO` keyword, `AST_DO_WHILE_STATEMENT` node,
parser production, pretty-printer case, and codegen emitter were added.
The code generator pushes a `(break, continue)` label pair onto the
existing loop stack; the continue label is positioned between the body
and the condition check, so `continue` jumps to the condition
evaluation.

Test results: 123/123 pass (102 valid + 12 invalid + 9 print_ast). No
regressions. Five new valid tests and one new print_ast test added.

Artifacts:
- docs/kickoffs/stage-10-02-kickoff.md
- docs/milestones/stage-10-02-summary.md
- docs/sessions/stage-10-02-transcript.md
- docs/grammar.md synced
