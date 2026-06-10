# ClaudeC99 Stage 100 — File-Scope Constant Expressions

## Goal

Wire the existing `eval_const_expr` evaluator into the file-scope integer-typed
variable initializer path so that standard C patterns compile at global scope:

```c
int PAGE_SIZE = 1 << 12;              /* shift     */
int MASK      = FLAG_A | FLAG_B;      /* bitwise   */
int BUF_SZ    = sizeof(int) * 1024;  /* sizeof    */
int NEG       = -(3 * 4);            /* arithmetic */
```

These are currently rejected with `"error: non-constant initializer for global"`.
The evaluator already handles all needed operators (stage 99).  Two concrete
changes are needed:

1. **Extend `eval_const_expr` with `sizeof(type-name)` support** — the only
   missing operator in the evaluator that is commonly used in file-scope
   initializers.

2. **Replace the literal-only check in `parse_external_declaration`** with a
   call to `eval_const_expr` for integer-typed global variable initializers.

This is a **language feature stage**.  No driver, lifecycle, or self-host
infrastructure changes.

---

## Background — what already works

File-scope variables are currently accepted with literal or trivially-constant
initializers only:

```c
int x   = 5;         /* works — INT_LITERAL                         */
char c  = 'A';       /* works — CHAR_LITERAL                        */
char *s = "hello";   /* works — STRING_LITERAL (pointer only)       */
enum { STEP = 5 };
int n   = STEP;      /* works — enum const folded to INT_LITERAL    */
```

The full `eval_const_expr` evaluator — supporting `* / % << >> + - & ^ |` with
unary `~ !` and parenthesized sub-expressions (stage 99) — is already used by:

- `case` label constants
- `enum` enumerator values
- array designator indices

The `parse_external_declaration` function calls `parse_assignment_expression`
for the non-brace initializer arm, then validates that the resulting AST node
is `AST_INT_LITERAL`, `AST_CHAR_LITERAL`, or `AST_STRING_LITERAL` (lines
~3643–3661 in `src/parser.c`).  Any expression node fails this check.

A secondary path, used for the second and later declarators in a comma-separated
list (`int a = 1, b = 2;` at file scope), calls `parse_primary` with the same
literal-only validation (lines ~3708–3716).

### `sizeof` is the missing operator

The single operator class not yet in `eval_const_expr` is `sizeof`.  It appears
frequently in file-scope initializers:

```c
static int buf_words = sizeof(int) * 1024;
static int ptr_bytes = sizeof(void *) * MAX_PTRS;
```

Because `sizeof` values are fully known at compile time, `sizeof(type-name)` is
a natural extension to `eval_const_primary`.

---

## Grammar

No new tokens.  Informally, the grammar for integer-typed file-scope variable
initializers is extended from:

```
<file-scope-initializer> ::= <integer-literal> | <char-literal>
```

to:

```
<file-scope-initializer> ::= <constant-integer-expression>
```

where `<constant-integer-expression>` is the same grammar used by `case`
labels and enumerator values (stage 99), extended with `sizeof(type-name)` in
the primary position.

---

## Task 1 — Extend `eval_const_primary` with `sizeof(type-name)`

In `src/parser.c`, in `eval_const_primary`, add handling for `TOKEN_SIZEOF`
**before** the fall-through error at the end of the function.

The logic mirrors the sizeof dispatch in `parse_primary` (lines ~1690–1740):

```
TOKEN_SIZEOF detected:
  consume sizeof
  if next is not TOKEN_LPAREN:
      PARSER_ERROR "error: sizeof requires a parenthesized type name in a constant expression\n"
  consume '('
  if current token starts a type-name:
      call parse_type_name(parser)  → Type *t
      if t is an incomplete array type → PARSER_ERROR
      parser_expect TOKEN_RPAREN
      return (long)type_size(t)
  else:
      PARSER_ERROR "error: sizeof requires a type name in a constant expression context\n"
```

**Type-start condition** (same tokens checked in `parse_primary`'s sizeof arm):

```c
parser->current.type == TOKEN_CONST     ||
parser->current.type == TOKEN_VOLATILE  ||
parser->current.type == TOKEN_BOOL      ||
parser->current.type == TOKEN_CHAR      ||
parser->current.type == TOKEN_SHORT     ||
parser->current.type == TOKEN_INT       ||
parser->current.type == TOKEN_LONG      ||
parser->current.type == TOKEN_SIGNED    ||
parser->current.type == TOKEN_UNSIGNED  ||
parser->current.type == TOKEN_STRUCT    ||
parser->current.type == TOKEN_UNION     ||
parser->current.type == TOKEN_ENUM      ||
(parser->current.type == TOKEN_IDENTIFIER &&
 parser_find_typedef(parser, parser->current.value))
```

Note: `TOKEN_ENUM` is added here (it was absent from `parse_primary`'s sizeof
arm — a pre-existing gap).  `sizeof(enum Color)` is always `sizeof(int)` and
is commonly written in constant-expression contexts.

**`parse_type_name`** is already a static function in `src/parser.c`; it is
callable from `eval_const_primary` directly.  **`type_size`** is declared in
`include/type.h` and returns `t->size` (an `int`).

`sizeof(void)` is already excluded because the type-start condition does not
include `TOKEN_VOID`, and `parse_type_name` rejects void further down.

---

## Task 2 — Wire `eval_const_expr` to the first-declarator file-scope path

In `src/parser.c`, inside `parse_external_declaration`, locate the non-brace
initializer arm (currently lines ~3642–3661):

```c
} else {
    /* Stage 98: use parse_assignment_expression … */
    init = parse_assignment_expression(parser);
    if (init->type == AST_COMPOUND_LITERAL) {
        PARSER_ERROR(parser,
                "error: compound literals at file scope are not yet supported\n");
    }
    if (init->type != AST_INT_LITERAL && init->type != AST_CHAR_LITERAL &&
        init->type != AST_STRING_LITERAL) {
        PARSER_ERROR(parser,
                "error: non-constant initializer for global '%s'\n", d.name);
    }
    if (init->type == AST_STRING_LITERAL && decl->decl_type != TYPE_POINTER) {
        PARSER_ERROR(parser,
                "error: string literal can only initialize a pointer\n");
    }
}
```

Replace with a branch on the declaration type:

```c
} else if (decl->decl_type != TYPE_POINTER) {
    /* Stage 100: integer-typed global — evaluate as a compile-time constant. */
    long val = eval_const_expr(parser, "file-scope initializer");
    char init_buf[32];
    snprintf(init_buf, sizeof(init_buf), "%ld", val);
    init = parser_node(parser, AST_INT_LITERAL,
               lexer_store_bytes(parser->lexer, init_buf, strlen(init_buf)));
} else {
    /* Pointer-typed global — keep literal-only path (NULL = 0, string ptrs). */
    init = parse_assignment_expression(parser);
    if (init->type == AST_COMPOUND_LITERAL) {
        PARSER_ERROR(parser,
                "error: compound literals at file scope are not yet supported\n");
    }
    if (init->type != AST_INT_LITERAL && init->type != AST_CHAR_LITERAL &&
        init->type != AST_STRING_LITERAL) {
        PARSER_ERROR(parser,
                "error: non-constant initializer for global '%s'\n", d.name);
    }
    if (init->type == AST_STRING_LITERAL) {
        /* string literal already OK for pointer — no extra check needed */
    }
}
```

**Why not a separate `is_integer_type` helper**: `decl->decl_type != TYPE_POINTER`
covers all remaining cases that reach this arm because struct/union are handled
by the `TOKEN_LBRACE` branch above, and arrays have their own section.

The `AST_INT_LITERAL` node produced here is identical to what the existing
codegen path for integer global initializers already handles — no codegen
changes are required.

---

## Task 3 — Wire `eval_const_expr` to the multi-declarator file-scope path

In `parse_external_declaration`, the comma-separated declarator list arm
(currently lines ~3708–3716):

```c
if (parser->current.type == TOKEN_ASSIGN) {
    parser->current = lexer_next_token(parser->lexer);
    ASTNode *init2 = parse_primary(parser);
    if (init2->type != AST_INT_LITERAL && init2->type != AST_CHAR_LITERAL) {
        PARSER_ERROR(parser,
                "error: non-constant initializer for global '%s'\n", d2.name);
    }
    ast_add_child(next_decl, init2);
}
```

Replace with the same branch:

```c
if (parser->current.type == TOKEN_ASSIGN) {
    parser->current = lexer_next_token(parser->lexer);
    ASTNode *init2;
    if (k2 != TYPE_POINTER) {
        /* Stage 100: integer-typed — evaluate as compile-time constant. */
        long val2 = eval_const_expr(parser, "file-scope initializer");
        char init2_buf[32];
        snprintf(init2_buf, sizeof(init2_buf), "%ld", val2);
        init2 = parser_node(parser, AST_INT_LITERAL,
                    lexer_store_bytes(parser->lexer, init2_buf, strlen(init2_buf)));
    } else {
        init2 = parse_primary(parser);
        if (init2->type != AST_INT_LITERAL && init2->type != AST_CHAR_LITERAL) {
            PARSER_ERROR(parser,
                    "error: non-constant initializer for global '%s'\n", d2.name);
        }
    }
    ast_add_child(next_decl, init2);
}
```

(`k2` is the TypeKind already computed for the second declarator at this point.)

---

## Out of scope — do NOT do these in this stage

- **Block-scope `static` variable initializers** — the local declaration path
  calls `parse_initializer` → `parse_assignment_expression` and does not
  validate that the result is a constant.  For non-static locals this is
  correct (runtime initializers are fine).  For `static` locals the codegen
  is responsible for emitting a constant initial value; the current behavior
  and its limitations are unchanged.  Fixing block-scope static initializers
  is a separate stage.
- **`sizeof(expression)`** — only `sizeof(type-name)` is supported in the
  constant expression evaluator.  `sizeof(var)` and `sizeof(1 + 2)` are
  rejected with a clear error.
- **Pointer-typed global initializers** beyond what already works — function
  pointer names, address-constant expressions (`&global_var`), and cast-from-
  integer initializers remain unsupported.
- **Relational, equality, logical, and ternary operators in `eval_const_expr`**
  — no change to the evaluator's operator set beyond sizeof.
- **Floating-point constant expressions** — out of scope project-wide.
- **`const`-qualified globals as compile-time constants** — in C99, `const int`
  file-scope variables are not integer constant expressions.  `eval_const_expr`
  correctly rejects identifiers that are not enum constants.

---

## Bootstrap caveats

The only new code path is: `eval_const_expr` called from a site that previously
called `parse_assignment_expression`.  The evaluator itself is already
self-hosting-verified from stage 99.  `sizeof(type-name)` calls
`parse_type_name`, which is also already self-hosting-verified.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Valid tests

**Basic arithmetic:**
```c
int x = 10 * 3 + 2;
int main() { return x - 32; }   /* expect 0 */
```

**Bitwise OR:**
```c
int MASK = 0xF0 | 0x0F;
int main() { return MASK - 255; }   /* expect 0 */
```

**Left shift:**
```c
int PAGE = 1 << 12;
int main() { return PAGE - 4096; }   /* expect 0 */
```

**sizeof(type):**
```c
int PTR_SZ = sizeof(void *);
int main() { return PTR_SZ - 8; }   /* expect 0 on LP64 */
```

**sizeof in expression:**
```c
int BUF = sizeof(int) * 256;
int main() { return BUF - 1024; }   /* expect 0 */
```

**sizeof struct:**
```c
struct Pair { int a; int b; };
int SZ = sizeof(struct Pair);
int main() { return SZ - 8; }   /* expect 0 */
```

**Enum constant with operator:**
```c
enum { STEP = 5 };
int LIMIT = STEP * 2 + 1;
int main() { return LIMIT - 11; }   /* expect 0 */
```

**Unary negation:**
```c
int NEG = -(3 * 4);
int main() { return NEG + 12; }   /* expect 0 */
```

**Bitwise complement:**
```c
int ALL = ~0;
int main() { return (ALL & 0xFF) == 0xFF ? 0 : 1; }   /* expect 0 */
```

**Comma-separated declarator list:**
```c
int A = 1 << 2, B = 3 * 7;
int main() { return A + B - 25; }   /* expect 0 */
```

### Invalid tests

**File-scope variable reference (not a constant expression):**
```c
int n = 5;
int m = n + 1;   /* error: n is not an integer constant expression */
int main() { return 0; }
```

**sizeof without parenthesized type:**
```c
int x = sizeof 5;   /* error: sizeof requires a parenthesized type name */
int main() { return 0; }
```

### Print-AST tests

At minimum one print-AST test confirming that a non-trivial expression is
folded to `IntLiteral` (not left as a binary/unary node) in the AST:

```c
int x = 1 + 2;
int main() { return x; }
```

Expected: the global declaration for `x` has `IntLiteral: 3` as its child
(constant-folded at parse time, not a binary operation node).

---

## Implementation order

1. `src/parser.c` — add `TOKEN_SIZEOF` handling to `eval_const_primary`;
   include `TOKEN_ENUM` in the type-start check (see Task 1).
2. `src/parser.c` — update the first-declarator file-scope initializer arm in
   `parse_external_declaration` (Task 2).
3. `src/parser.c` — update the multi-declarator file-scope initializer arm in
   `parse_external_declaration` (Task 3).
4. `src/version.c` — bump `VERSION_STAGE` to `"01000000"`.
5. Tests — add valid, invalid, and print_ast tests.
6. Documentation (see below).

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; add file-scope constant
  expressions (with `sizeof`) to the feature list; refresh test totals.
- **`docs/grammar.md`** — update `<file-scope-initializer>` for integer types
  to reference `<constant-integer-expression>`; add `sizeof(type-name)` to the
  `<constant-integer-expression>` primary.
- Run the **`update-supplemental-docs`** skill to refresh the feature checklist,
  parse-tree, and status/features snapshots through stage 100.
- **`docs/self-compilation-report.md`** — record the stage-100 self-host run.
