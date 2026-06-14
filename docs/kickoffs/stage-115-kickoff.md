# Stage 115 — Constant Expressions in Array Dimension Bounds

**Kickoff Document** — Plan before implementation

---

## 1. Summary of the Spec

This is a parser-only stage that extends array dimension parsing to accept full
C99 integer constant expressions instead of bare integer literals.

**Current behavior:** Array dimensions in any of four sites (`sizeof(int[N])`,
`int (*a)[N]`, `int a[N]`, or additional `[M]`) accept only `TOKEN_INT_LITERAL`.
When a macro expands to an arithmetic expression like `LIMIT + 1`, the parser
rejects it with "expected ']', got '+'".

**After this stage:** All four sites call `eval_const_expr(parser, "array dimension")`,
which already implements the full C99 constant-expression grammar (arithmetic,
bitwise, shift, relational, equality, logical AND/OR, ternary, `sizeof`,
parentheses, enum constants, macro identifiers).

**Scope:** Parser only. No token changes, no AST changes, no codegen changes.

**Tests:** 6 valid tests + 2 invalid tests. No bootstrap modifications expected
(the compiler's own source uses only literal constants in array dimensions).

---

## 2. Required Tokenizer Changes

**None.** No new tokens or lexer modifications needed.

---

## 3. Required Parser Changes

Four sites in `src/parser.c` must be modified to replace literal-only reads with
calls to `eval_const_expr()`.

### Site A — `parse_type_name` bracket loop (~line 1122)
Used for `sizeof(int[N+1])`, compound-literal type-names, `va_arg` type-names.
Replace:
```c
if (parser->current.type != TOKEN_INT_LITERAL) {
    PARSER_ERROR(parser, "error: array size must be an integer literal\n");
}
int length = (int)parser->current.long_value;
parser->current = lexer_next_token(parser->lexer);
```
With:
```c
long length = eval_const_expr(parser, "array dimension");
```
Preserve the empty-dimension `continue` path (when token is `]` at loop start).

### Site B — `parse_direct_declarator` parenthesized form (~line 1207)
Used for `int (*a)[N]` and similar.
Replace:
```c
if (parser->current.type == TOKEN_INT_LITERAL) {
    Token size_tok = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    int length = (int)size_tok.long_value;
    ...
} else if (parser->current.type != TOKEN_RBRACKET) {
    PARSER_ERROR(parser, "error: array size must be an integer literal\n");
}
```
With:
```c
if (parser->current.type != TOKEN_RBRACKET) {
    long length = eval_const_expr(parser, "array dimension");
    ...
}
```

### Site C — `parse_direct_declarator` non-paren first dimension (~line 1288)
Used for `int a[N]`, `int a[N][M]`, `typedef int Row[N]`, `struct S { int a[N]; }`.
Same pattern: replace literal-only block with `if (parser->current.type != TOKEN_RBRACKET) { long length = eval_const_expr(...); ... }`.

### Site D — `parse_direct_declarator` non-paren additional dimensions (~line 1309)
Used for second and subsequent dimensions in `int a[N][M]`.
No empty-bracket case here (additional dimensions cannot be omitted).
Replace:
```c
if (parser->current.type != TOKEN_INT_LITERAL) {
    PARSER_ERROR(parser, "error: array size must be an integer literal\n");
}
Token size_tok = parser->current;
parser->current = lexer_next_token(parser->lexer);
int length = (int)size_tok.long_value;
```
With:
```c
long length = eval_const_expr(parser, "array dimension");
```

---

## 4. Required AST Changes

**None.** The `declarator` struct already stores array dimensions as integers;
`eval_const_expr` evaluates the expression and returns a `long` that is cast to
`int` and stored as before.

---

## 5. Required Code Generation Changes

**None.** Array dimensions are resolved at parse time; codegen is unchanged.

---

## 6. Test Plan

### Valid tests (6 total)

1. **`test/valid/declarations/test_global_array_dim_macro_plus_1__42.c`**
   - `#define LIMIT 41; static int a[LIMIT + 1]; ... return a[0];` → expect 42

2. **`test/valid/declarations/test_global_array_dim_multiply__42.c`**
   - `#define ROWS 6; #define COLS 7; static int grid[ROWS * COLS]; ... return grid[0];` → expect 42

3. **`test/valid/arrays/test_local_array_dim_const_expr__42.c`**
   - `#define BASE 40; int a[BASE + 2]; ... return a[0];` → expect 42

4. **`test/valid/arrays/test_multi_array_dim_const_expr__42.c`**
   - `#define ROWS 3; #define COLS 4; int a[ROWS + 1][COLS + 2]; ... return a[0][0];` → expect 42

5. **`test/valid/declarations/test_typedef_array_dim_const_expr__42.c`**
   - `#define N 10; typedef int Row[N * 2]; Row r; ... return r[0];` → expect 42

6. **`test/valid/structs/test_struct_member_array_dim_const_expr__42.c`**
   - `#define SZ 5; struct Buf { int data[SZ + 5]; }; ... return b.data[0];` → expect 42

7. **`test/valid/expressions/test_sizeof_array_dim_const_expr__20.c`**
   - `#define N 5; int sz = (int)sizeof(int[N * 4]); return sz - 60;` → expect 20

### Invalid tests (2 total)

1. **`test/invalid/arrays/test_array_dim_runtime_var__`** (error)
   - `int n = 5; int a[n];` → expect error containing "is not an integer constant expression"

2. **`test/invalid/arrays/test_array_dim_negative_const_expr__`** (error)
   - `#define NEG -1; int a[NEG + 0];` → expect error containing "array size must be greater than zero"

---

## Implementation Order

1. Apply Site A fix in `parse_type_name`
2. Apply Site B fix in `parse_direct_declarator` (parenthesized)
3. Apply Sites C and D fixes in `parse_direct_declarator` (non-paren)
4. Build and run `test/valid/run_tests.sh` — verify all pass
5. Add the 7 valid test files
6. Add the 2 invalid test files
7. Run full suite `test/run_all_tests.sh` — verify all pass
8. Bump `src/version.c` to `"01150000"`
9. Self-host: `./build.sh --mode=self-host` (C0→C1→C2 cycle)
10. Update `docs/self-compilation-report.md`
11. Commit with trailer `Co-Authored-By: sgflicker@gmail.com`
12. Delegate README + milestone + transcript to `haiku-stage-artifact-writer`

---

## Notes and Decisions

- All changes are isolated to `src/parser.c`. No ripple effects expected.
- The `eval_const_expr` function has been available since stage 99 and is
  battle-tested in self-hosting cycles.
- The compiler's own source uses only literals in array dimensions, so the
  bootstrap cycle will be unaffected; no compiler modifications expected.
- Empty-dimension cases (`int a[]`, `(*a)[]`) are preserved by the new
  `if (current != RBRACKET)` guards.
- The evaluation preserves the existing "array size must be greater than zero"
  check after `eval_const_expr` returns.
