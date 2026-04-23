# Stage-11-05-01 Milestone: Typed Parameter and Return Grammar

Extended the function and parameter grammar so function declarations
carry a declared return type and parameters carry a declared parameter
type. The accepted integer types are `char`, `short`, `int`, and
`long` — the same set already supported for variable declarations.

## What was completed
- Grammar: `<function>` and `<parameter_declaration>` now accept any
  `<integer_type>` in place of the previously hardcoded `int`.
- Parser: added `parser_expect_integer_type` helper and threaded the
  chosen `TypeKind` onto `AST_FUNCTION_DECL.decl_type` (return type)
  and `AST_PARAM.decl_type` (parameter type).
- AST pretty printer: `FunctionDecl` and `Parameter` lines now include
  the declared type (e.g. `FunctionDecl: long sum`,
  `Parameter: char a`).
- Regenerated all 12 existing `test/print_ast/*.expected` files to
  match the new printed format.
- Added new print-AST test `test_stage_11_05_01_typed_func` covering
  non-`int` return types, non-`int` parameter types, and a forward
  declaration form.
- Updated `docs/grammar.md` to reflect the new productions.

## Scope limits (per spec)
No tokenizer or codegen changes. No call-site parameter-type checking,
no return-value conversions, and no additional signature-compatibility
checks beyond the existing arity check.

## Test results
All 190 tests pass (163 valid + 14 invalid + 13 print_ast). No
regressions.
