# Stage-12-04 Milestone: Pointer Parameters and Arguments

## Completed
- Parameter declarations may now be pointer-typed: `<parameter_declaration> ::= <type> <identifier>`, where `<type> ::= <integer_type> { "*" }`.
- Pointer-valued expressions (variable references, address-of, dereference) may be passed as arguments to a pointer parameter.
- Pointer parameters are stored as 8-byte locals; existing dereference and assign-through-pointer codegen works inside callees with no change.
- The call site rejects integer/pointer mismatches in either direction and rejects mismatched pointer base chains (e.g. `char *` vs `int *`).
- AST pretty-printer renders pointer parameters using the same C-style notation as pointer declarations.

## Files Changed
- `src/parser.c` — `parse_parameter_declaration` now consumes `<integer_type> { "*" } <identifier>` and annotates AST_PARAM with `decl_type` / `full_type`.
- `src/ast_pretty_printer.c` — AST_PARAM prints pointer types via `ast_print_type`.
- `src/codegen.c` — parameter prologue passes `full_type` into `codegen_add_var`; call-site argument handling adds pointer/integer mismatch and pointer-base-equality checks (helper `pointer_types_equal`).
- `docs/grammar.md` — `<parameter_declaration>` updated to `<type> <identifier>`.
- Tests: 3 valid, 3 invalid, 1 print-AST.

## Test Results
- Valid: 200 / 200 pass.
- Invalid: 22 / 22 pass.
- Print-AST: 16 / 16 pass.
- No regressions.
