# Stage-15-02 Kickoff

## Spec
`docs/stages/ClaudeC99-spec-stage-15-02-adding-character-literal-expression-node.md`

## Stage Goal
Wire `TOKEN_CHAR_LITERAL` (already produced by the lexer in stage 15-01)
into the parser, AST, and code generator so that a character literal is
a valid `<primary_expression>`. Per C99 semantics, a character constant
has type `int`, so `'A'` evaluates as `65`, `'\n'` as `10`, etc.

## Delta from Stage 15-01
Stage 15-01 was lexer-only: the token was emitted and printed but no
`<primary_expression>` rule consumed it. Stage 15-02 introduces
`AST_CHAR_LITERAL`, accepts `TOKEN_CHAR_LITERAL` in `parse_primary`,
emits the integer constant from `codegen_expression`, and extends the
AST pretty printer.

## Ambiguities Flagged
- **AST payload shape.** The spec only requires the new node type. To
  match the existing `AST_STRING_LITERAL` convention (and the 15-01
  token convention), the AST node stores the decoded byte at
  `node->value[0]` (length 1, NUL at `value[1]`); the integer value is
  derived as `(unsigned char)node->value[0]`. `decl_type` is set to
  `TYPE_INT` per C semantics. No new fields are added to `ASTNode`.
- **Pretty-print format.** `IntLiteral` prints source text;
  `StringLiteral` re-escapes decoded bytes. For `CharLiteral` the plan
  is `CharLiteral: '<re-escaped>' (<int>)` so spelling and value are
  both visible (e.g. `CharLiteral: '\n' (10)`).
- **Grammar tag name.** Spec uses `<char_literal>`; the existing
  `docs/grammar.md` uses `<character_literal>`. The existing tag is
  kept for consistency.

## Planned Changes

### Tokenizer (`include/token.h`, `src/lexer.c`)
- No changes (lexer already produces `TOKEN_CHAR_LITERAL`).

### AST (`include/ast.h`)
- Add `AST_CHAR_LITERAL` to `ASTNodeType`.

### Parser (`src/parser.c`)
- New `TOKEN_CHAR_LITERAL` branch in `parse_primary` that constructs
  an `AST_CHAR_LITERAL` node with the decoded byte at `value[0]`
  (length 1) and `decl_type = TYPE_INT`.

### AST pretty printer (`src/ast_pretty_printer.c`)
- Add `case AST_CHAR_LITERAL`: print
  `CharLiteral: '<re-escaped spelling>' (<int>)` using the same
  escape table as the string-literal case extended with `'`.

### Code generator (`src/codegen.c`)
- Handle `AST_CHAR_LITERAL` in `codegen_expression`: emit
  `mov eax, <int>` and set `result_type = TYPE_INT`.
- Handle `AST_CHAR_LITERAL` in `expr_result_type`: return `TYPE_INT`.

### Tests
- `test/print_ast/test_stage_15_02_char_literal.c` (+ `.expected`).
- `test/valid/test_char_literal_uppercase_A__65.c`
- `test/valid/test_char_literal_digit_0__48.c`
- `test/valid/test_char_literal_escape_n__10.c`
- `test/valid/test_char_literal_escape_t__9.c`
- `test/valid/test_char_literal_escape_r__13.c`
- `test/valid/test_char_literal_escape_backslash__92.c`
- `test/valid/test_char_literal_escape_squote__39.c`
- `test/valid/test_char_literal_escape_dquote__34.c`
- `test/valid/test_char_literal_escape_null__0.c`
- `test/valid/test_char_literal_arith__66.c`

### Documentation
- `docs/grammar.md`: extend `<primary_expression>` with
  `| <character_literal>`.
- `README.md`: bump "Through stage 15-01" to "Through stage 15-02";
  update the character-literal bullet to reflect parser/AST/codegen
  support; refresh aggregate test totals.

### Commit
Single commit at the end of the stage.
