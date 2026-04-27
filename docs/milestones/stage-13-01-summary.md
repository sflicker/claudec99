# Stage-13-01 Milestone: Array Types

## Completed
- New `TYPE_ARRAY` kind in the type system, carrying the element
  type, length, and total byte size.
- New tokens `TOKEN_LBRACKET` / `TOKEN_RBRACKET` and lexing of `[` / `]`.
- Local declarations accept `<type> <identifier> "[" <integer_literal> "]" ";"`
  for any existing scalar element type and for pointer-to-scalar
  element types (e.g. `int a[3];`, `char a[4];`, `long b[2];`,
  `short s[4];`, `int *ptrs[2];`).
- Codegen allocates `sizeof(element) * length` bytes on the stack
  for an array local, aligned to the element's natural alignment.
- The locals table records arrays the same way scalars are tracked
  (name, rbp-relative offset, size, kind, full type chain).
- Constraints enforced by the parser:
  - Array size must be an integer literal.
  - Array size must be greater than zero (`int a[0]` rejected).
  - `int a[-1]` is rejected (the `-` is not an integer literal).
  - `int a[x]` is rejected (identifier is not an integer literal).
  - `int a[3] = ...;` is rejected (array initializers out of scope).
- Codegen rejects use of an array variable as a value
  (`AST_VAR_REF` of an array local) and rejects assignment to an
  array local (`a = b;`), implementing the "arrays are not
  assignable" rule.
- Pretty printer renders array declarations as
  `VariableDeclaration: <element-type> <name>[<length>]`.
- Grammar doc updated for the new `<declaration>` production.

## Files Changed
- `include/token.h`, `src/lexer.c` — `TOKEN_LBRACKET`,
  `TOKEN_RBRACKET` and lexing.
- `include/type.h`, `src/type.c` — `TYPE_ARRAY`, `length` field,
  `type_array(element_type, length)` constructor, extended
  `type_kind_name` / `type_is_integer`.
- `src/parser.c` — array bracket suffix in `<declaration>`,
  positive-integer-literal validation, rejection of array
  initializers.
- `src/codegen.c` —
  - `type_kind_bytes(TYPE_ARRAY)` returns 0 (size lives on the
    array Type's `size` field).
  - `codegen_add_var` gets an explicit `align` parameter so array
    locals can align to element size instead of total byte count.
  - `compute_decl_bytes` accounts for actual array byte count plus
    alignment slack.
  - `AST_DECLARATION` for `TYPE_ARRAY` allocates the slot with no
    initializer evaluation.
  - `AST_VAR_REF` and `AST_ASSIGNMENT` reject array locals.
- `src/ast_pretty_printer.c` — `<element-type> <name>[<length>]`
  rendering for array declarations.
- `docs/grammar.md` — updated `<declaration>` production.
- `test/valid/` — 5 new tests.
- `test/invalid/` — 5 new tests.
- `test/print_ast/` — 1 new test + expected.

## Test Results
- Valid: 214 / 214 pass.
- Invalid: 35 / 35 pass.
- Print-AST: 18 / 18 pass.
- No regressions.
