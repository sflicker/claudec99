# Milestone Summary

## Stage-15-02: Character literal expression node — Complete

- Added `AST_CHAR_LITERAL` to the AST node enum. The parser now
  accepts `TOKEN_CHAR_LITERAL` as a `<primary_expression>`,
  constructing a node with the decoded byte at `value[0]` (length 1)
  and `decl_type = TYPE_INT` (per C semantics).
- Code generator emits `mov eax, <int>` for an `AST_CHAR_LITERAL`
  expression and reports `TYPE_INT` from `expr_result_type`, so the
  literal participates in arithmetic, comparison, return, and
  assignment exactly like an integer constant.
- AST pretty printer renders the node as
  `CharLiteral: '<re-escaped>' (<int>)`, e.g. `CharLiteral: '\n' (10)`.
- Tokenizer is unchanged from stage 15-01.
- Tests added:
  - 1 print_ast test (`test_stage_15_02_char_literal`).
  - 10 valid tests covering each supported form: `'A'` (65), `'0'`
    (48), `'\n'` (10), `'\t'` (9), `'\r'` (13), `'\\'` (92), `'\''`
    (39), `'\"'` (34), `'\0'` (0), and `'A' + 1` (66) for arithmetic.
- Documentation updated: `docs/grammar.md` extends
  `<primary_expression>` with `| <character_literal>` and drops the
  "lexer-only" caveat from the lexical comment. `README.md` bumped to
  "Through stage 15-02" with the character-literal bullet replaced
  and the aggregate test totals refreshed (428 = 262 valid + 60
  invalid + 24 print-AST + 74 print-tokens + 8 print-asm).
- Full suite: 428/428 pass; no regressions.
