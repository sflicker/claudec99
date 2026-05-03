# Stage 17-02 Kickoff — sizeof Expressions

## Derived Stage Values

- STAGE_LABEL: `stage-17-02`
- STAGE_DISPLAY: `Stage 17-02`

## Spec Summary

Extends `sizeof` from type-name-only operands (stage 17-01) to expression operands:

- `sizeof <unary_expression>` — no parens (e.g. `sizeof x`)
- `sizeof(<expression>)` — parenthesized expression (e.g. `sizeof(x + y)`)
- `sizeof(<type>)` — existing path; unchanged

The expression operand is **never evaluated**. No code is emitted for it. The
compiler resolves the expression type statically, computes the size as a
compile-time constant, and emits `mov rax, <size>`. The result type of
`sizeof` remains `long` (TYPE_LONG), consistent with stage 17-01.

Integer promotions and usual arithmetic conversions apply to the operand type
resolution: `sizeof(char_var + 1)` → 4 (char promotes to int), 
`sizeof(int_var + long_var)` → 8 (long wins).

## Spec Issues

1. Title typo: "sizeof expresions" (missing 's').
2. Line 81: missing closing paren in inline example (`sizeof(A[0]` → `sizeof(A[0])`).
3. Test cases at lines 228 and 234 are missing semicolons after pointer declarations (`char *p` → `char *p;`). Test files will include the corrected form.
4. Lines 32–48: third `int main()` example is missing its closing `}` before the closing triple-backtick (formatting only; intent is clear).

## Required Changes

### Tokenizer
No changes. `TOKEN_SIZEOF` already exists.

### Parser (`src/parser.c`)
Rework the `sizeof` branch in `parse_unary`:
- After consuming `sizeof`:
  - If next token is NOT `(`: parse a `<unary_expression>` as the operand → create `AST_SIZEOF_EXPR`.
  - If next token IS `(`: consume `(`, then peek:
    - If next is a type keyword (`char`, `short`, `int`, `long`): take the existing `AST_SIZEOF_TYPE` path.
    - If next is `)`: error — empty sizeof.
    - Otherwise: parse `<expression>`, expect `)`, create `AST_SIZEOF_EXPR`.

### AST (`include/ast.h`)
Add `AST_SIZEOF_EXPR` to the `ASTNodeType` enum (after `AST_SIZEOF_TYPE`).

### Code Generation (`src/codegen.c`)
- New static helper `sizeof_type_of_expr(cg, node)` → `TypeKind`:
  - Returns the raw declared type for var refs (no promotion).
  - Returns the pointee type for dereferences.
  - Returns the element type for array subscripts.
  - Applies promotions and arithmetic conversions for binary ops.
  - Does NOT emit any code.
- Add `AST_SIZEOF_EXPR` to `expr_result_type` → `TYPE_LONG`.
- Add `AST_SIZEOF_EXPR` case to `codegen_expression`: call `sizeof_type_of_expr` on the child, compute the size, emit `mov rax, <size>`.

### Pretty Printer (`src/ast_pretty_printer.c`)
Add `case AST_SIZEOF_EXPR: printf("SizeofExpr:\n"); break;`

### Tests
- ~15 valid test files covering: basic vars, non-paren form, arithmetic type rules, pointer vars, pointer dereference, array element, side-effect suppression.
- 3 invalid test files: `sizeof;` (bare), `sizeof()` (empty parens), `sizeof(1 +)` (malformed expression).

### Documentation
- `docs/grammar.md`: Update `<unary_expression>` to add `"sizeof" <unary_expression>` alternative.
- `README.md`: Update stage description to reflect expression operand support.

## Implementation Order

1. `include/ast.h` — add `AST_SIZEOF_EXPR`
2. `src/parser.c` — extend sizeof parsing
3. `src/codegen.c` — add `sizeof_type_of_expr`, wire into `expr_result_type` and `codegen_expression`
4. `src/ast_pretty_printer.c` — add pretty print case
5. `test/valid/` — new valid test files
6. `test/invalid/` — new invalid test files
7. `docs/grammar.md` — grammar update
8. `README.md` — update

## Out of Scope

- `sizeof(A)` whole array
- `sizeof(int[10])` type-with-array-dimension
- VLAs, structs, unions, enums, `double`, `float`, `size_t`
