# Stage stage-17-01 Kickoff

## Stage
stage-17-01 — sizeof Type Names

## Spec Summary
Add `sizeof(<type>)`: sizeof applied to an explicit type name inside parentheses.
- Supported: `sizeof(char)` → 1, `sizeof(short)` → 2, `sizeof(int)` → 4, `sizeof(long)` → 8, `sizeof(int *)` → 8
- Result type: `long` (emitted as a 64-bit constant in rax)
- Out of scope: sizeof applied to expressions (`sizeof x`, `sizeof (x + 1)`)

## Spec Issues
- Minor doc error: in the Examples section, `sizeof(int)` is listed twice; the fourth entry should be `sizeof(long)`. Test cases are correct. Not a blocker.

## Required Changes

### Tokenizer
- Add `TOKEN_SIZEOF` to `include/token.h`
- Recognize `"sizeof"` as a keyword in `src/lexer.c`

### Parser
- In `parse_unary` (`src/parser.c`): detect `TOKEN_SIZEOF`, consume it, expect `(`, parse base type keyword, consume any `*` tokens (pointer), expect `)`, build `AST_SIZEOF_TYPE` node with `decl_type` holding the TypeKind
- Error if `sizeof` is not followed by `( <type> )`

### AST
- Add `AST_SIZEOF_TYPE` to `include/ast.h`
- Node stores the type via `decl_type` (TYPE_CHAR/SHORT/INT/LONG/POINTER); no children needed

### Code Generation
- `expr_result_type`: return `TYPE_LONG` for `AST_SIZEOF_TYPE`
- `codegen_expression`: emit `mov rax, N` where N = 1/2/4/8 per type

### Tests
- `test_sizeof_char__1.c` → expect exit 1
- `test_sizeof_short__2.c` → expect exit 2
- `test_sizeof_int__4.c` → expect exit 4
- `test_sizeof_long__8.c` → expect exit 8
- `test_sizeof_int_ptr__8.c` → expect exit 8

### Grammar
- Update `<unary_expression>` in `docs/grammar.md`

## Implementation Order
1. token.h (TOKEN_SIZEOF)
2. lexer.c (keyword recognition)
3. ast.h (AST_SIZEOF_TYPE)
4. parser.c (parse_unary sizeof branch)
5. codegen.c (expr_result_type + codegen_expression)
6. test files
7. docs/grammar.md
8. README.md
