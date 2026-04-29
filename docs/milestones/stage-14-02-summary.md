# Stage-14-02 Milestone: String Literal AST Node

## Completed

- Added `AST_STRING_LITERAL` to `ASTNodeType`. The new node carries the
  literal's body bytes on `node->value` and its byte length implicitly
  via `full_type = type_array(type_char(), N + 1)`, where `N` is the
  source byte count and the trailing element holds the implicit NUL.
- Extended `parse_primary` to accept `TOKEN_STRING_LITERAL` as a
  `<primary_expression>` alternative. The new branch consumes the
  token, builds the AST node, and assigns it the `char[N+1]` array
  type per the spec's type rule.
- Updated the AST pretty-printer to render the new node as
  `StringLiteral: "<bytes>" (length N)` so the preserved byte length
  is visible and empty strings print unambiguously.
- Added two `print_ast` tests covering a basic literal (`"hi"`) and
  the empty literal (`""`).
- Updated `docs/grammar.md` with the new `<string_literal>` production
  and the extended `<primary_expression>` alternative.

No lexer, codegen, or token changes — emitting strings, taking their
addresses, escape sequences, and any runtime behavior remain out of
scope for this stage.

## Files Changed

- `include/ast.h` — added `AST_STRING_LITERAL`.
- `src/parser.c` — `parse_primary` accepts `TOKEN_STRING_LITERAL` and
  builds `AST_STRING_LITERAL` with `char[N+1]` full_type.
- `src/ast_pretty_printer.c` — added the `AST_STRING_LITERAL` case.
- `test/print_ast/test_stage_14_02_string_literal_basic.c` (+`.expected`).
- `test/print_ast/test_stage_14_02_string_literal_empty.c` (+`.expected`).
- `docs/grammar.md` — added `<string_literal>` and updated
  `<primary_expression>`.

## Test Results

- Valid:        232 / 232 pass.
- Invalid:       44 /  44 pass.
- Print-AST:     21 /  21 pass (+2).
- Print-Tokens:  72 /  72 pass.
- Aggregate:    369 / 369 pass. No regressions.
