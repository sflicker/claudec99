# Stage-11-06 Kickoff: Explicit Casts Between Integer Types

## Spec Summary

Stage 11-06 adds explicit cast expressions over the four supported
signed integer types (`char`, `short`, `int`, `long`). A cast has the
form `"(" <integer_type> ")" <cast_expression>`. The grammar gains a
new production `<cast_expression>` sitting between
`<multiplicative_expression>` and `<unary_expression>` — multiplicative
now consumes cast expressions, and a cast either falls through to a
unary expression or applies a target integer type to another (right-
recursive) cast expression.

Semantically:

- The type of a cast expression is its target type.
- Narrowing truncates (keeping the low byte/word/dword).
- Widening sign-extends.
- Same-size casts do not change the representation but still produce
  the target type.

Cast codegen must reuse the same widen/narrow helper already shared by
assignment, parameter passing, and return — no new conversion
machinery.

## Delta From Previous Stage (11-05-03)

Stage 11-05-03 finalized conversions at the function boundary
(arguments, parameter stores, returns) using `emit_convert`. This
stage exposes that same conversion mechanism to programmers via an
explicit `(type)expr` syntax. The AST gains one new node
(`AST_CAST`); the parser gains one new production (`parse_cast`); the
pretty-printer, expression type analysis, and code generator each
grow one branch to handle the new node. No tokenizer changes.

## Spec Observations (noted, not blocking)

1. The spec's `<unary_expression>` production (re-quoted in the
   grammar block) recurses into `<unary_expression>`, not
   `<cast_expression>`. Standard C lets a unary operator apply to a
   cast (e.g., `-(int)x`), but under the grammar as written this stage
   does not support that form. The listed tests do not require it, so
   I will follow the spec literally and leave `parse_unary` recursing
   into itself.

2. The spec requires the cast AST node to "track source/from type" as
   a distinct attribute. In the current AST the source type is already
   available through `children[0]->result_type` (set during codegen,
   the same field used by every existing conversion site). To keep the
   AST minimal — per project notes — I will rely on that field rather
   than adding a new struct member. The target type is recorded in the
   node's `decl_type`.

Neither observation blocks implementation.

## Planned Changes

### Tokenizer
- No changes.

### Parser
- Add `parse_cast(Parser *)` implementing
  `<cast_expression> ::= <unary_expression> | "(" <integer_type> ")"
  <cast_expression>`. Uses the existing save/restore lookahead idiom:
  when the current token is `(`, consume it and check whether the
  next token is an integer-type keyword. If yes, consume the type,
  expect `)`, and recurse into `parse_cast` for the operand. If no,
  restore lexer state and fall through to `parse_unary` so
  parenthesized expressions still route through `parse_primary`.
- Update `parse_term` (multiplicative) to call `parse_cast` in place
  of `parse_unary`.
- Forward-declare `parser_expect_integer_type` or inline the type-
  token check locally; either is fine (I will inline to minimize
  dependency changes).

### AST
- Add `AST_CAST` to `ASTNodeType` (include/ast.h).
- No new `ASTNode` fields: `decl_type` holds the target type;
  `children[0]` holds the source expression; the source type is the
  child's `result_type`.
- Update `ast_pretty_print` (src/ast_pretty_printer.c) to handle
  `AST_CAST`, printing the target type (e.g., `Cast: int`).

### Code Generator
- Add an `AST_CAST` branch in `codegen_expression`:
  1. Emit code for `children[0]`.
  2. Call `emit_convert(cg, children[0]->result_type, node->decl_type)`.
  3. Set `node->result_type = node->decl_type`.
- Add an `AST_CAST` case in `expr_result_type` returning
  `node->decl_type` so binary-op common-type analysis sees the cast
  as its target type.

### Tests
Add six valid tests (one per spec bullet):
- `test_cast_long_to_char__2.c` — narrowing `long 258 → char`, low
  byte is `2`.
- `test_cast_long_to_short__0.c` — narrowing `long 65536 → short`,
  low 16 bits are `0`.
- `test_cast_char_to_long__5.c` — widening `char 5 → long`.
- `test_cast_in_arithmetic__42.c` — cast participates in a binop,
  e.g. `(int)b + a` where `b` is long, `a` is int.
- `test_cast_nested__42.c` — `(int)(char)(long)42`.
- `test_cast_function_call_result__2.c` — `(char)f()` where `f`
  returns `long 258` → low byte `2`.

### Documentation
- Update `docs/grammar.md`: add `<cast_expression>` production and
  change the right-hand side of `<multiplicative_expression>` from
  `<unary_expression>` to `<cast_expression>`.

### Commit
- Single commit at end of stage.
