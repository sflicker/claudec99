# Stage-12-05 Milestone: Pointer Return Types

## Completed
- Function return types may now be pointers: `<function> ::= <type> <identifier> "(" [<parameter_list>] ")" ( <block_statement> | ";" )`, where `<type> ::= <integer_type> { "*" }`.
- Pointer return values flow through `rax` (8-byte) without integer conversion. Return statements enforce strict matching: integer-vs-pointer mismatches and mismatched pointer chains are rejected.
- Function-call expressions carry the callee's pointer chain on `full_type` so downstream uses (declarations, returns) see the correct type.
- Declaration initializers reject pointer/non-pointer mismatches and incompatible pointer chains.
- AST pretty-printer renders pointer return types using the same C-style notation as pointer declarations and parameters.

## Files Changed
- `src/parser.c` — `parse_function_decl` now consumes `<integer_type> { "*" }` and annotates `AST_FUNCTION_DECL` with `decl_type` / `full_type`. Removed unused helper `parser_expect_integer_type`.
- `src/codegen.c` — `AST_FUNCTION_CALL` propagates the callee's `full_type`; `AST_RETURN_STATEMENT` enforces strict pointer typing and skips integer conversion on the pointer path; `AST_DECLARATION` enforces pointer/non-pointer and chain compatibility on initializers; `current_return_full_type` field tracks the current function's pointer return chain.
- `include/codegen.h` — added `current_return_full_type` to `CodeGen`.
- `src/ast_pretty_printer.c` — `AST_FUNCTION_DECL` prints pointer return types via `ast_print_type`.
- `docs/grammar.md` — `<function>` rule updated to use `<type>` for the return.
- Tests: 2 valid, 6 invalid, 1 print-AST.

## Test Results
- Valid: 202 / 202 pass.
- Invalid: 28 / 28 pass.
- Print-AST: 17 / 17 pass.
- No regressions.
