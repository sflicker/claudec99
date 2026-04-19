# Milestone Summary — stage-09-05

Stage 9.5 is complete. The compiler now supports function declarations
(prototypes) as a new form of external declaration. A `<function>` header
ending in `;` is a declaration; ending in `{...}` is a definition. Both
register the function's signature in the parser's table, so a call is
valid whenever either form is visible at the call site. This enables
forward calls that stage 9.4 rejected — a prototype before `main`, with
the definition appearing later in the translation unit.

The parser now enforces:
- **Parameter-count consistency** across all declarations and the
  definition for a given function name.
- **At most one definition** per name (`duplicate function definition`).
- **Multiple redundant declarations** are permitted as long as their
  parameter counts agree.

Code generation skips any `AST_FUNCTION_DECL` node whose children list
contains no `AST_BLOCK` body — declarations emit no assembly.

Test results: 109/109 pass (91 valid + 10 invalid + 8 print_ast). No
regressions. Four new valid tests and one new invalid test.

Artifacts:
- docs/kickoffs/stage-09-05-kickoff.md
- docs/milestones/stage-09-05-summary.md
- docs/sessions/stage-09-05-transcript.md
- docs/grammar.md synced
