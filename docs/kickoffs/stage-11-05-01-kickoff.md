# Stage-11-05-01 Kickoff: Typed Parameter and Return Grammar

## Spec Summary
Stage 11-05-01 extends the grammar, parser, AST, and AST pretty printer
so that function declarations carry a declared return type and function
parameters carry a declared parameter type. The supported integer types
are the same four already recognized for variable declarations:
`char`, `short`, `int`, `long`. Codegen is untouched, no type checking
is performed on calls or return values, and the existing arity checks
are preserved. The only end-user-visible surface change is the AST
print output: function and parameter nodes now show their declared
types.

## What Must Change From Stage 11-04
- `<function>` production: the leading `"int"` keyword is replaced by
  `<integer_type>`.
- `<parameter_declaration>` production: the leading `"int"` keyword is
  replaced by `<integer_type>`.
- Parser records the declared type on `AST_FUNCTION_DECL` and
  `AST_PARAM` via the existing `ASTNode.decl_type` field.
- AST pretty printer prints `FunctionDecl: <type> <name>` and
  `Parameter: <type> <name>`.
- Existing `.expected` print-AST files (which hardcode the old format
  without a type) must be regenerated.
- `docs/grammar.md` must reflect the new productions.

## Out of Scope
Function-call expression typing, argument-to-parameter conversions,
return-value conversions, function signature compatibility checks
beyond existing arity checks, and any codegen changes.

## Spec Notes / Ambiguity
- The spec EBNF references `<parameter_list>` without redefining it;
  the existing production in `docs/grammar.md` is unchanged, so this is
  an omission not a change.
- Reusing the existing `TypeKind decl_type` field on `ASTNode` matches
  the variable-declaration pretty-print convention
  (`VariableDeclaration: int i`) and requires no AST-structure edits.
- Because call-site parameter-type checking and return-value
  conversions are explicitly out of scope, the `FuncSig` table does
  not need parameter-type fields.

## Planned Changes
- **Tokenizer**: none.
- **Parser**: accept any of `char | short | int | long` at the start
  of a function definition / declaration and at the start of a
  parameter declaration; store the chosen `TypeKind` on the
  `AST_FUNCTION_DECL` / `AST_PARAM` node's `decl_type`.
- **AST**: none (reuse existing `decl_type`).
- **AST pretty printer**: update `FunctionDecl` and `Parameter` lines
  to include the declared type.
- **Codegen**: none.
- **Tests**: regenerate all 12 existing `test/print_ast/*.expected`
  files and add a new `test_stage_11_05_01_typed_func` pair covering
  non-`int` return and parameter types plus a forward declaration.
- **Grammar**: update `<function>` and `<parameter_declaration>` in
  `docs/grammar.md`.
