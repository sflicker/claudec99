# ClaudeC99 Stage 109 — `float`/`double` Types, Literals, and Stack Variables

## Goal

Introduce `float` and `double` as first-class types across the lexer, type
system, parser, and code generator, stopping short of arithmetic and
function calls. After this stage a program can declare, initialize, and
copy float/double variables; it cannot yet perform arithmetic, compare
float values, or pass them as function arguments.

```c
float  x = 1.5f;
double y = 3.14;
double z = x;        /* implicit widening */
float  f;
f = (float)y;        /* explicit cast — Stage 110 */
```

No new AST control-flow nodes. No changes to the calling convention.
No `long double` (deferred indefinitely).

---

## Background — current gap

`float` and `double` are unrecognised identifiers. Any declaration of either
type hits `"error: expected type specifier"`. Float literals like `1.5` or
`3.14f` parse as `1` followed by `.5` (i.e. field access on an integer —
a different parse error).

---

## C99 references

| Feature | C99 section |
|---------|-------------|
| `float` / `double` type specifiers | §6.7.2 |
| Floating constants | §6.4.4.2 |
| `sizeof` of real floating types | §6.5.3.4 |
| Implicit conversions between real types | §6.3.1.5 |

---

## Task 1 — Tokens (`include/token.h`, `src/lexer.c`)

### 1a — New tokens

In `include/token.h`, add after `TOKEN_VOID`:

```c
TOKEN_FLOAT,
TOKEN_DOUBLE,
```

In `src/lexer.c`, in the keyword recognition block:

```c
else if (strcmp(ident_buf.data, "float")  == 0) {
    token.type = TOKEN_FLOAT;  token.value = "float";  token.value_len = 5;
}
else if (strcmp(ident_buf.data, "double") == 0) {
    token.type = TOKEN_DOUBLE; token.value = "double"; token.value_len = 6;
}
```

Add to `token_type_name()`:

```c
case TOKEN_FLOAT:  return "'float'";
case TOKEN_DOUBLE: return "'double'";
```

### 1b — Float/double literal lexing

After the integer literal path in `lexer_next_token`, extend the digit
sequence scanner to detect floating-point constants.

A token is a floating-point literal when a digit sequence is followed by:
- `.` (decimal point), OR
- `e` or `E` (exponent), OR
- a `f`/`F` suffix after digits with a decimal point.

**Lexing algorithm** (modify the existing integer-literal scanning block):

1. Scan an initial digit sequence (as for integers).
2. If the next char is `.` and the char after is NOT `.` (to avoid
   `1..2` ambiguity) and is NOT a letter other than `e`/`E`: consume `.`
   and continue scanning digits.
3. If the next char is `e` or `E`: consume it, optionally consume `+`/`-`,
   then scan exponent digits.
4. If after all digits/exponent the next char is `f` or `F`: consume it;
   token is `TOKEN_FLOAT_LITERAL`.
5. If a `.` or `e`/`E` was consumed but no `f`/`F`: token is
   `TOKEN_DOUBLE_LITERAL`.
6. If only digits were scanned with no `.` or `e`/`E`: token remains
   `TOKEN_INT_LITERAL` (existing path; no change).

A leading `.` followed by digits (e.g. `.5`) shall also produce a
`TOKEN_DOUBLE_LITERAL` (or `TOKEN_FLOAT_LITERAL` with `f` suffix):
detect this in the initial character dispatch before the digit sequence
branch.

Store the raw text (including suffix) in `token.value`; `token.value_len`
includes the suffix character. The raw text is passed to NASM; NASM parses
the decimal value directly from `DD`/`DQ` directives (see Task 4).

Add to `include/token.h`:

```c
TOKEN_FLOAT_LITERAL,
TOKEN_DOUBLE_LITERAL,
```

Add to `token_type_name()`:

```c
case TOKEN_FLOAT_LITERAL:  return "float literal";
case TOKEN_DOUBLE_LITERAL: return "double literal";
```

---

## Task 2 — Type system (`include/type.h`, `src/type.c`)

### 2a — New type kinds

In `include/type.h`, add to the `TypeKind` enum:

```c
TYPE_FLOAT,
TYPE_DOUBLE,
```

`TYPE_FLOAT` and `TYPE_DOUBLE` are scalar types with no sub-fields.
They follow the existing singleton-pointer pattern used by `TYPE_INT`,
`TYPE_LONG`, etc.

### 2b — Type accessors

In `src/type.c`, add singleton functions:

```c
Type *type_float(void)  { static Type t = {TYPE_FLOAT};  return &t; }
Type *type_double(void) { static Type t = {TYPE_DOUBLE}; return &t; }
```

### 2c — `sizeof` and alignment

In the existing `type_size` / `type_align` functions (or wherever the
compiler resolves these — check `src/codegen.c` or `src/parser.c`):

```c
case TYPE_FLOAT:  return 4;   /* size and align */
case TYPE_DOUBLE: return 8;
```

### 2d — `type_is_fp` predicate

Add a helper used throughout codegen:

```c
static int type_is_fp(Type *t) {
    return t && (t->kind == TYPE_FLOAT || t->kind == TYPE_DOUBLE);
}
```

---

## Task 3 — Parser (`src/parser.c`)

### 3a — `parse_type_specifier`

Add `TOKEN_FLOAT` and `TOKEN_DOUBLE` cases parallel to `TOKEN_INT`:

```c
} else if (parser->current.type == TOKEN_FLOAT) {
    parser->current = lexer_next_token(parser->lexer);
    return type_float();
} else if (parser->current.type == TOKEN_DOUBLE) {
    parser->current = lexer_next_token(parser->lexer);
    return type_double();
```

Also add `TOKEN_FLOAT` and `TOKEN_DOUBLE` to any lookahead sets that
test "is this a type specifier?" (type-name detection in `parse_cast`,
`parse_postfix`, and the block-scope declaration dispatch in
`parse_statement`).

### 3b — `parse_primary` — float/double literals

After the `TOKEN_INT_LITERAL` branch, add:

```c
} else if (parser->current.type == TOKEN_FLOAT_LITERAL ||
           parser->current.type == TOKEN_DOUBLE_LITERAL) {
    ASTNode *node = ast_make_node(
        parser->current.type == TOKEN_FLOAT_LITERAL
            ? AST_FLOAT_LITERAL : AST_FLOAT_LITERAL);
    node->value       = parser->current.value;
    node->value_len   = parser->current.value_len;
    node->decl_type   = (parser->current.type == TOKEN_FLOAT_LITERAL)
                        ? TYPE_FLOAT : TYPE_DOUBLE;
    node->result_type = (node->decl_type == TYPE_FLOAT)
                        ? type_float() : type_double();
    parser->current = lexer_next_token(parser->lexer);
    return node;
```

### 3c — New AST node type

In `include/ast.h`, add:

```c
AST_FLOAT_LITERAL,
```

`node->value` holds the raw literal text (e.g. `"1.5f"`, `"3.14"`).
`node->decl_type` is `TYPE_FLOAT` or `TYPE_DOUBLE`.

### 3d — Implicit widening assignment (float → double only)

In the assignment and declaration-initializer paths, permit assigning a
`float` rvalue to a `double` lvalue without an explicit cast (C99 §6.3.1.5).
The codegen will emit `cvtss2sd`. The reverse (double → float) requires an
explicit cast (narrowing loses precision) — reject silently or warn, per
existing project style.

> This is the only implicit FP conversion needed in Stage 109; full UAC
> for arithmetic is deferred to Stage 110.

---

## Task 4 — Codegen (`src/codegen.c`)

### 4a — FP result register convention

FP expression values land in `xmm0`; integer/pointer values continue to
land in `rax`. The codegen for any node whose `result_type` is `TYPE_FLOAT`
or `TYPE_DOUBLE` must leave its result in `xmm0` rather than `rax`.

No new fields in `CodeGen` are required for Stage 109 — only load/store and
literal reference are emitted, and these always leave the value in `xmm0`.

### 4b — Stack frame allocation for FP locals

In the local-variable registration path (wherever `LocalVar` slots are
assigned), use `type_size(t)` and `type_align(t)` for float/double
variables. Stack offsets must be naturally aligned (4-byte for float,
8-byte for double). The existing alignment rounding already handles this
if `type_size`/`type_align` return the correct values.

### 4c — Float/double load (`AST_VAR_REF` with FP type)

When emitting a variable reference for an FP-typed local:

```nasm
movss xmm0, [rbp - N]   ; float
movsd xmm0, [rbp - N]   ; double
```

When emitting a variable reference for an FP-typed global
(RIP-relative):

```nasm
movss xmm0, [rel Lglobal_name]
movsd xmm0, [rel Lglobal_name]
```

### 4d — Float/double store (`AST_ASSIGNMENT` / `AST_DECLARATION` init)

After evaluating the RHS into `xmm0`, store to the LHS:

```nasm
movss [rbp - N], xmm0   ; float local
movsd [rbp - N], xmm0   ; double local
```

### 4e — Float literals in `.rodata`

Float/double literals cannot be encoded as immediate operands in SSE
instructions (unlike integer immediates). Each unique literal is emitted
as a label in `.rodata`:

```nasm
section .rodata
Lfc_0:    DD 1.5        ; float  — NASM accepts decimal after DD
Lfc_1:    DQ 3.14       ; double — NASM accepts decimal after DQ
```

NASM converts the decimal value to IEEE 754 representation automatically.
The `f`/`F` suffix must be stripped from the value before emitting to
NASM (emit `1.5`, not `1.5f`).

Reference the literal with a RIP-relative load:

```nasm
movss xmm0, [rel Lfc_0]
movsd xmm0, [rel Lfc_1]
```

Literal deduplication: maintain a small table (a `Vec` of `{raw_text,
label_index}` pairs) per translation unit. Before emitting, search the
table; reuse the label if an identical text string is already present.
(Two textually identical literals that happen to denote the same value
share a label; two textually distinct literals with the same IEEE 754
value get separate labels — this is conservative but correct.)

### 4f — Implicit float-to-double widening

When a `TYPE_FLOAT` value is assigned to a `TYPE_DOUBLE` slot:

```nasm
cvtss2sd xmm0, xmm0
movsd [rbp - N], xmm0
```

---

## Task 5 — Version bump (`src/version.c`)

```c
#define VERSION_STAGE  "01090000"
```

---

## Implementation order

1. `include/token.h` — `TOKEN_FLOAT`, `TOKEN_DOUBLE`, `TOKEN_FLOAT_LITERAL`, `TOKEN_DOUBLE_LITERAL`
2. `src/lexer.c` — keyword recognition; float/double literal lexing
3. `include/type.h` / `src/type.c` — `TYPE_FLOAT`, `TYPE_DOUBLE`, `type_float()`, `type_double()`, `type_is_fp()`
4. `include/ast.h` — `AST_FLOAT_LITERAL`
5. `src/parser.c` — `parse_type_specifier`; `parse_primary` for literals; type-start lookahead sets; implicit widening
6. `src/codegen.c` — stack allocation; load/store; `.rodata` literal table; implicit widening codegen
7. `src/version.c` — version bump
8. Tests
9. Build, full test suite, self-host cycle

---

## Out of scope — do NOT do these in this stage

- **FP arithmetic** (`+`, `-`, `*`, `/`, unary `-`) — Stage 110.
- **FP comparisons** (`<`, `==`, etc.) — Stage 111.
- **Implicit int → float/double conversion in expressions** — Stage 110.
- **FP function parameters and return values** — Stage 112.
- **`va_arg` for `double`** — Stage 112.
- **`<math.h>` stub** — Stage 112.
- **`long double`** — deferred indefinitely.
- **`_Complex` / `_Imaginary`** — deferred indefinitely.
- **Hex float literals** (`0x1.8p1`) — defer; decimal covers all practical tests.

---

## Tests

### `test_float_declare__0.c` — declare and assign a float literal

```c
int main(void) {
    float x = 1.0f;
    (void)x;
    return 0;
}
```

Expected exit: 0.

### `test_double_declare__0.c` — declare and assign a double literal

```c
int main(void) {
    double y = 3.14;
    (void)y;
    return 0;
}
```

Expected exit: 0.

### `test_float_copy__0.c` — copy between float variables

```c
int main(void) {
    float a = 2.0f;
    float b = a;
    (void)b;
    return 0;
}
```

Expected exit: 0.

### `test_double_widen__0.c` — implicit float → double widening

```c
int main(void) {
    float f = 1.0f;
    double d = f;
    (void)d;
    return 0;
}
```

Expected exit: 0.

### `test_float_struct_member__0.c` — float as a struct member

```c
struct Point { float x; float y; };
int main(void) {
    struct Point p;
    p.x = 1.5f;
    p.y = 2.5f;
    (void)p;
    return 0;
}
```

Expected exit: 0.

### `test_double_global__0.c` — global double variable

```c
double g = 2.718;
int main(void) {
    double d = g;
    (void)d;
    return 0;
}
```

Expected exit: 0.

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; note `float`/`double`
  types and literals are now supported; refresh test totals.
- **`docs/grammar.md`** — add `float` and `double` to the type-specifier
  grammar; add floating-constant to the literal grammar.
- **`docs/outlines/checklist.md`** — add Stage 109 section.
- Run the **`update-supplemental-docs`** skill.
- **`docs/self-compilation-report.md`** — record Stage 109 self-host run.
