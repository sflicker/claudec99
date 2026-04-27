# Stage-13-02 Milestone: Array Indexing

## Completed
- New AST node `AST_ARRAY_INDEX` with children `[base, index]`.
- Parser extends `<postfix_expression>` to accept the subscript
  suffix `"[" <expression> "]"` after a primary, building an
  `AST_ARRAY_INDEX` wrapping the base and the index expression. The
  base must be an identifier in this stage; pointer indexing is out
  of scope.
- Assignment-LHS lvalue check accepts `AST_ARRAY_INDEX` so
  `a[i] = expr` parses.
- Codegen emits the address `base + index * sizeof(element)` into
  rax via a shared helper:
  - `lea` the array's stack address.
  - Evaluate the index expression; require integer type; sign-extend
    to 64 bits; multiply by element size (skipped when size is 1).
  - Add the base address.
- Subscript read (`a[i]` as rvalue) loads through the computed
  address using the element type's width: byte/word loads sign-
  extend into `eax`, 4-byte uses `mov eax`, 8-byte uses `mov rax`.
- Subscript write (`a[i] = rhs`) computes the address, stashes it,
  evaluates the RHS, converts it to the element type using the
  existing `emit_convert`, and stores at the element width.
- `expr_result_type` returns the (promoted) element type for
  `AST_ARRAY_INDEX`, so binary arithmetic over `a[i]` operands picks
  the correct common type.
- Pretty printer renders `AST_ARRAY_INDEX` as `ArrayIndex:`.
- `docs/grammar.md` updated for the new subscript alternative in
  `<postfix_expression>`.

## Files Changed
- `include/ast.h` — new `AST_ARRAY_INDEX`.
- `src/parser.c` — subscript branch in `parse_postfix`; lvalue
  acceptance in `parse_expression`.
- `src/codegen.c` —
  - `emit_array_index_addr` helper (forward-declares
    `codegen_expression`).
  - `AST_ASSIGNMENT` LHS path for `AST_ARRAY_INDEX`.
  - `AST_ARRAY_INDEX` rvalue path mirroring `AST_DEREF`.
  - `expr_result_type` case for `AST_ARRAY_INDEX`.
- `src/ast_pretty_printer.c` — `ArrayIndex:` rendering.
- `docs/grammar.md` — subscript suffix in `<postfix_expression>`.
- `test/valid/` — 3 new tests (the three spec tests, exits 15/30/42).
- `test/print_ast/` — 1 new test + expected.

## Test Results
- Valid: 217 / 217 pass.
- Invalid: 35 / 35 pass.
- Print-AST: 19 / 19 pass.
- No regressions.
