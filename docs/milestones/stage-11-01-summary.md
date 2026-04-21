# Stage-11-01 Milestone: Minimal Type System

Introduced a minimal type-system module (`type.h` / `type.c`) and lexer,
parser, and AST support for the integer-type keywords `char`, `short`,
`int`, `long` in local variable declarations.

## What was completed

- New tokens `TOKEN_CHAR`, `TOKEN_SHORT`, `TOKEN_LONG` and matching
  lexer keyword recognition.
- New types module with a `TypeKind` enum, `Type` struct, and stub
  helpers (`type_char`, `type_short`, `type_int`, `type_long`,
  `type_kind_name`, `type_size`, `type_alignment`, `type_is_integer`).
- `ASTNode` carries a `TypeKind decl_type` field, populated by the
  parser for variable declarations.
- Parser accepts any of `char | short | int | long` as the leading
  token of a local declaration; parameter, return, and function-declaration
  grammar is unchanged.
- AST printer renders the declared type on a `VariableDeclaration`
  line as `VariableDeclaration: <type> <name>`.
- New tests: one print-AST test covering all four types, three
  exit-code tests covering single-type declarations, and one covering
  a mixed-type sum.
- Grammar doc updated to reflect the new `<integer-type>` rule.
- Codegen is unchanged: all four types are still emitted as 32-bit
  `int` locals.

## Test results

154 tests pass (12 print_ast + 128 valid + 14 invalid). No regressions.
