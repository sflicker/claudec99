# ClaudeC99 Stage 115 — Constant Expressions in Array Dimension Bounds

## Goal

Extend the array-dimension parser to accept the full integer constant-expression
syntax wherever a fixed array size is written: declarators, `sizeof` type-names,
typedef declarations, and struct/union member arrays.

```c
/* All of these must compile after this stage. */
#define LIMIT 65000
static char composite[LIMIT + 1];    /* file-scope: was "expected ']', got '+'" */

#define ROWS 3
#define COLS 4
int grid[ROWS + 1][COLS + 2];        /* multidimensional */

typedef int Matrix[ROWS * 2][COLS * 2];   /* typedef array bound */

struct Buffer {
    char data[LIMIT + 1];            /* struct member */
};

int sz = sizeof(int[LIMIT + 1]);     /* sizeof type-name */
```

All of these are valid C99 (§6.7.5.2 — the dimension expression shall be an
"integer constant expression or an expression that is not an integer constant
expression" for VLAs; only the constant-expression form is targeted here).
Currently each is rejected because the dimension parser accepts only a bare
`TOKEN_INT_LITERAL`, erroring when it sees the `+` or `*` that follows a
macro-expanded integer literal.

This is a **parser-only stage**.  No AST changes, no codegen changes, no new
tokens or grammar rules.

---

## Background — current behavior

The parser has four distinct sites where an array dimension is consumed:

| Site | Location | Context |
|------|----------|---------|
| A | `parse_type_name` — bracket loop | `sizeof(int[N])`, casts, `va_arg` |
| B | `parse_direct_declarator` — parenthesized form | `int (*a)[N]` |
| C | `parse_direct_declarator` — non-paren, first dim | `int a[N]`, `int a[N][M]` |
| D | `parse_direct_declarator` — non-paren, additional dims | `int a[N][M]` second+ |

All four sites contain the same pattern:

```c
if (parser->current.type != TOKEN_INT_LITERAL) {
    PARSER_ERROR(parser, "error: array size must be an integer literal\n");
}
int length = (int)parser->current.long_value;
parser->current = lexer_next_token(parser->lexer);
```

The token stream after macro expansion of `LIMIT + 1` is:
`INT_LITERAL(65000)  PLUS  INT_LITERAL(1)  RBRACKET`.

The parser consumes `65000` as the dimension, then calls
`parser_expect(parser, TOKEN_RBRACKET)` — which finds `+`, not `]`, and dies:

```
error: expected ']', got '+' ('+')
```

The fix for all four sites is the same: replace the literal-only read with a
call to the existing `eval_const_expr(parser, "array dimension")`, which already
handles the full C99 integer constant-expression grammar (stages 99–104):
arithmetic, bitwise, shift, relational, equality, logical AND/OR, ternary,
`sizeof`, parentheses, enum constants, and macro-expanded identifiers.

---

## Task 1 — Four parser fixes in `src/parser.c`

### Site A — `parse_type_name` bracket loop (~line 1122)

Used for `sizeof(int[N+1])`, compound-literal type-names (`(int[N+1]){...}`),
`va_arg` type-names.

The loop body after consuming `[` currently reads:

```c
if (dim_count == 0 && parser->current.type == TOKEN_RBRACKET) {
    /* Omitted first dimension — stage 98 compound literal (int[]). */
    parser->current = lexer_next_token(parser->lexer);
    dims[dim_count++] = 0;
    continue;
}
if (parser->current.type != TOKEN_INT_LITERAL) {
    PARSER_ERROR(parser, "error: array size must be an integer literal\n");
}
int length = (int)parser->current.long_value;
parser->current = lexer_next_token(parser->lexer);
if (length <= 0) {
    PARSER_ERROR(parser, "error: array size must be greater than zero\n");
}
dims[dim_count++] = length;
parser_expect(parser, TOKEN_RBRACKET);
```

Replace the literal-only block with:

```c
if (dim_count == 0 && parser->current.type == TOKEN_RBRACKET) {
    /* Omitted first dimension — stage 98 compound literal (int[]). */
    parser->current = lexer_next_token(parser->lexer);
    dims[dim_count++] = 0;
    continue;
}
long length = eval_const_expr(parser, "array dimension");
if (length <= 0) {
    PARSER_ERROR(parser, "error: array size must be greater than zero\n");
}
dims[dim_count++] = (int)length;
parser_expect(parser, TOKEN_RBRACKET);
```

The empty-dimension `continue` path is preserved unchanged; `eval_const_expr`
is only reached when the token after `[` is not `]`.

### Site B — `parse_direct_declarator` parenthesized form (~line 1207)

Used for `int (*a)[N]` and similar parenthesized-star declarators.

Current code after consuming `[`:

```c
if (parser->current.type == TOKEN_INT_LITERAL) {
    Token size_tok = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    int length = (int)size_tok.long_value;
    if (length <= 0) {
        PARSER_ERROR(parser, "error: array size must be greater than zero\n");
    }
    d.array_length = length;
    d.has_size = 1;
} else if (parser->current.type != TOKEN_RBRACKET) {
    PARSER_ERROR(parser, "error: array size must be an integer literal\n");
}
parser_expect(parser, TOKEN_RBRACKET);
```

Replace with:

```c
if (parser->current.type != TOKEN_RBRACKET) {
    long length = eval_const_expr(parser, "array dimension");
    if (length <= 0) {
        PARSER_ERROR(parser, "error: array size must be greater than zero\n");
    }
    d.array_length = (int)length;
    d.has_size = 1;
}
parser_expect(parser, TOKEN_RBRACKET);
```

The empty-bracket case (`(*a)[]`) was already handled by the `else if != RBRACKET`
falling through; the new code reproduces that by only calling `eval_const_expr`
when the next token is not `]`.

### Site C — `parse_direct_declarator` non-paren first dimension (~line 1288)

Used for plain variable, typedef, parameter, and struct-member declarators:
`int a[N]`, `int a[N][M]`, `typedef int Row[N]`, `struct S { int a[N]; }`.

Current code after consuming `[`:

```c
if (parser->current.type == TOKEN_INT_LITERAL) {
    Token size_tok = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    int length = (int)size_tok.long_value;
    if (length <= 0) {
        PARSER_ERROR(parser, "error: array size must be greater than zero\n");
    }
    d.array_dims[d.array_dim_count++] = length;
    d.array_length = length;
    d.has_size = 1;
} else if (parser->current.type != TOKEN_RBRACKET) {
    PARSER_ERROR(parser, "error: array size must be an integer literal\n");
}
parser_expect(parser, TOKEN_RBRACKET);
```

Replace with:

```c
if (parser->current.type != TOKEN_RBRACKET) {
    long length = eval_const_expr(parser, "array dimension");
    if (length <= 0) {
        PARSER_ERROR(parser, "error: array size must be greater than zero\n");
    }
    d.array_dims[d.array_dim_count++] = (int)length;
    d.array_length = (int)length;
    d.has_size = 1;
}
parser_expect(parser, TOKEN_RBRACKET);
```

The empty first-dimension case (`int a[]`) was already handled by the old
`else if != RBRACKET` falling through; preserved by the new `if != RBRACKET`
guard.

### Site D — `parse_direct_declarator` non-paren additional dimensions (~line 1309)

Used for second and subsequent dimensions in `int a[N][M]`, `int a[N][M][P]`,
etc.  Additional dimensions may never be omitted in C99, so there is no
empty-bracket special case here.

Current code after consuming `[`:

```c
if (parser->current.type != TOKEN_INT_LITERAL) {
    PARSER_ERROR(parser, "error: array size must be an integer literal\n");
}
Token size_tok = parser->current;
parser->current = lexer_next_token(parser->lexer);
int length = (int)size_tok.long_value;
if (length <= 0) {
    PARSER_ERROR(parser, "error: array size must be greater than zero\n");
}
d.array_dims[d.array_dim_count++] = length;
parser_expect(parser, TOKEN_RBRACKET);
```

Replace with:

```c
long length = eval_const_expr(parser, "array dimension");
if (length <= 0) {
    PARSER_ERROR(parser, "error: array size must be greater than zero\n");
}
d.array_dims[d.array_dim_count++] = (int)length;
parser_expect(parser, TOKEN_RBRACKET);
```

---

## Task 2 — `src/version.c`

Bump `VERSION_STAGE` to `"01150000"`.

---

## Implementation order

1. Apply Site A fix (`parse_type_name`).
2. Apply Site B fix (`parse_direct_declarator` parenthesized).
3. Apply Sites C and D fixes (`parse_direct_declarator` non-paren).
4. Build (`cmake --build build`).
5. Run `test/valid/run_tests.sh` — all must pass.
6. Add tests (see below).
7. Run full suite (`./test/run_all_tests.sh`) — all must pass.
8. Bump `src/version.c`.
9. Self-host: `./build.sh --mode=self-host` — all three passes must pass.
10. Update `docs/self-compilation-report.md`.
11. Commit.
12. Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.

---

## Out of scope — do NOT do these in this stage

- **Variable-length arrays (VLAs)**: `int a[n]` where `n` is a runtime
  variable is a C99 optional feature not targeted here.  After this change,
  `eval_const_expr` will produce `"is not an integer constant expression"` for
  a runtime identifier — that is the correct rejection for a compiler that does
  not support VLAs.
- **`static` and `restrict` qualifiers inside parameter array dimensions**:
  `void f(int a[static N])` and `void f(int a[restrict N])` are C99 additions
  to parameter declarators; the parser does not handle those qualifiers inside
  `[...]` and they remain out of scope.
- **`*` as VLA size sentinel in parameter declarations**: `void f(int a[*])` is
  a C99 prototype syntax for VLAs; not addressed here.
- **Function parameter array decay**: `int a[]` (empty first dim) already works
  in parameter position via the existing empty-bracket path; no change needed.
- **`eval_const_init` in `src/codegen.c`**: the block-scope static evaluator
  is not affected; array dimensions are resolved at parse time.
- **Grammar documentation update**: the `<constant-integer-expression>` rule in
  `docs/grammar.md` is unchanged (the evaluator grammar did not change; only
  where it is called changed).

---

## Bootstrap caveats

`eval_const_expr` has been available in the self-hosted compiler since stage 99.
The compiler's own source code uses only literal integer constants as array
dimensions (e.g. `int dims[MAX_ARRAY_DIMS]`, `int fp_param_types[FUNC_TYPE_MAX_PARAMS]`)
— never a macro arithmetic expression — so the bootstrap cycle will see
identical code paths before and after this change.  No compiler source
modifications are expected during the self-host run.

---

## Tests

Place new tests in `test/valid/` under the category that best fits.

### Valid — file-scope (global) arrays

**`test/valid/declarations/test_global_array_dim_macro_plus_1__42.c`**
```c
#define LIMIT 41
static int a[LIMIT + 1];
int main(void) { a[0] = 42; return a[0]; }
```
Expected exit: 42

**`test/valid/declarations/test_global_array_dim_multiply__42.c`**
```c
#define ROWS 6
#define COLS 7
static int grid[ROWS * COLS];
int main(void) { grid[0] = 42; return grid[0]; }
```
Expected exit: 42

### Valid — block-scope (local) arrays

**`test/valid/arrays/test_local_array_dim_const_expr__42.c`**
```c
#define BASE 40
int main(void) {
    int a[BASE + 2];
    a[0] = 42;
    return a[0];
}
```
Expected exit: 42

### Valid — multidimensional

**`test/valid/arrays/test_multi_array_dim_const_expr__42.c`**
```c
#define ROWS 3
#define COLS 4
int main(void) {
    int a[ROWS + 1][COLS + 2];
    a[0][0] = 42;
    return a[0][0];
}
```
Expected exit: 42

### Valid — typedef array bound

**`test/valid/declarations/test_typedef_array_dim_const_expr__42.c`**
```c
#define N 10
typedef int Row[N * 2];
int main(void) {
    Row r;
    r[0] = 42;
    return r[0];
}
```
Expected exit: 42

### Valid — struct member array bound

**`test/valid/structs/test_struct_member_array_dim_const_expr__42.c`**
```c
#define SZ 5
struct Buf { int data[SZ + 5]; };
int main(void) {
    struct Buf b;
    b.data[0] = 42;
    return b.data[0];
}
```
Expected exit: 42

### Valid — sizeof with constant-expression dimension

**`test/valid/expressions/test_sizeof_array_dim_const_expr__20.c`**
```c
#define N 5
int main(void) {
    int sz = (int)sizeof(int[N * 4]);
    return sz - 60;   /* sizeof(int[20]) = 80 bytes; 80 - 60 = 20 */
}
```
Expected exit: 20

### Invalid — runtime variable in array dimension (VLA not supported)

**`test/invalid/arrays/test_array_dim_runtime_var__`** (error)

```c
int main(void) {
    int n = 5;
    int a[n];
    return a[0];
}
```
Expected: compiler error containing `is not an integer constant expression`

### Invalid — negative constant expression in array dimension

**`test/invalid/arrays/test_array_dim_negative_const_expr__`** (error)

```c
#define NEG -1
int a[NEG + 0];
int main(void) { return 0; }
```
Expected: compiler error containing `array size must be greater than zero`

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — add "Through stage 115…" capability entry; update test
  totals.
- **`docs/self-compilation-report.md`** — add stage-115 self-host result.
- **`docs/grammar.md`** — no change (the grammar for `<constant-integer-expression>`
  is unchanged; only the contexts in which it is accepted changed).
- Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.

---

## Done criteria

All existing tests continue to pass.  The six valid tests above pass.  The two
invalid tests above are rejected with the expected error messages.
`./build.sh --mode=self-host` completes C0→C1→C2 with all tests passing at
each stage.
