# Stage-11-06 Milestone: Explicit Casts Between Integer Types

Added explicit cast expressions `(type)expr` over the four supported
signed integer types (`char`, `short`, `int`, `long`). A cast produces
a value of its target type; narrowing truncates and widening sign-
extends, matching the rules already used at assignment, argument, and
return boundaries.

## What was completed
- New AST node `AST_CAST` (include/ast.h). Target type is stored in
  the existing `decl_type` field; the source expression is `children[0]`
  and its source type is carried on the child's `result_type` during
  codegen. No new AST struct fields were required.
- New parser production `parse_cast` (src/parser.c) implementing
  `<cast_expression> ::= <unary_expression> | "(" <integer_type> ")"
  <cast_expression>`. Uses the save/restore lookahead idiom to
  disambiguate `(type)` casts from parenthesized expressions.
- `parse_term` (multiplicative) now consumes `parse_cast` instead of
  `parse_unary`.
- `ast_pretty_print` renders `AST_CAST` as `Cast: <target-type>`.
- Codegen (`codegen_expression`) evaluates the cast's source, invokes
  the existing `emit_convert(src_result_type, target)` helper, and
  sets the node's `result_type` to the target — reusing the same
  widen/narrow conversion logic already shared by assignment,
  parameter passing, and return.
- `expr_result_type` reports the cast's target type so it participates
  correctly in binary-op common-type analysis.
- Added six new valid tests covering the full spec testing matrix:
  long→char narrowing, long→short narrowing, char→long widening, a
  cast inside arithmetic, nested casts, and a cast around a function-
  call result.
- `docs/grammar.md` updated: new `<cast_expression>` production; the
  RHS of `<multiplicative_expression>` now references `<cast_expression>`.

## Scope limits (per spec)
No tokenizer changes. No new AST struct fields. Only signed integer
types are castable — unsigned casts, pointer casts, function/array/
struct casts, and full C `<type-name>` parsing remain out of scope.
The grammar as written keeps `<unary_expression>` recursing into
`<unary_expression>` (not `<cast_expression>`), matching the spec
literally; forms like `-(int)x` are not supported in this stage.

## Test results
All 205 tests pass (178 valid + 14 invalid + 13 print_ast), including
the 6 new cast tests. No regressions.
