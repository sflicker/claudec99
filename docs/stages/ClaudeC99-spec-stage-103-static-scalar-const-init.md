# ClaudeC99 Stage 103 — Block-Scope Static Scalar Constant-Expression Initializers

## Goal

Extend block-scope static scalar initializers to accept the full set of
compile-time constant expressions — the same operator set used by `case`
labels, enum values, and file-scope globals (stages 77, 99, 100):

```c
static int SHIFT  = 1 << 3;           /* shift               */
static int MASK   = 0xF0 | 0x0F;      /* bitwise OR          */
static long SZ    = sizeof(long) * 8; /* sizeof in expr      */
static int NEG    = -(3 * 4);         /* nested arithmetic   */
static int FLAGS  = FLAG_A | FLAG_B;  /* enum-constant refs  */
```

These are currently rejected with `"error: initializer for block-scope
static '%s' must be a constant expression"` even though they are valid C99.

This is a **codegen-only stage**.  No parser, AST, or grammar changes.

---

## Background — current state

Block-scope static scalar initializers are validated in the SC_STATIC arm
of `codegen_statement` (lines ~4377–4399 of `src/codegen.c`).  The current
check is:

```c
if (init->type == AST_INT_LITERAL) {
    init_value = strtol(init->value, NULL, 10);
    is_initialized = 1;
} else if (init->type == AST_CHAR_LITERAL) {
    init_value = (long)(unsigned char)init->value[0];
    is_initialized = 1;
} else if (init->type == AST_UNARY_OP &&
           strcmp(init->value, "-") == 0 &&
           init->child_count > 0 &&
           init->children[0]->type == AST_INT_LITERAL) {
    init_value = -strtol(init->children[0]->value, NULL, 10);
    is_initialized = 1;
} else {
    compile_error(
        "error: initializer for block-scope static '%s' must be a "
        "constant expression\n", node->value);
}
```

This handles three cases:
- `AST_INT_LITERAL` — integer or folded enum constant
- `AST_CHAR_LITERAL` — character literal
- `AST_UNARY_OP("-") + AST_INT_LITERAL` — literal negation (e.g. `-5`)

Anything else — including valid constant expressions like `3 * 4`, `1 << 8`,
or `FLAG_A | FLAG_B` (when not already folded by the parser) — hits the
`compile_error`.

The parser already correctly builds the AST for these expressions:
`3 * 4` becomes `AST_BINARY_OP("*", INT_LITERAL(3), INT_LITERAL(4))`,
`1 << 8` becomes `AST_BINARY_OP("<<", INT_LITERAL(1), INT_LITERAL(8))`,
etc.  No parser change is needed; the codegen just needs to evaluate the tree.

Note: enum constants are folded to `AST_INT_LITERAL` by the parser at the
use site, so `static int x = MY_ENUM;` already works via the first branch.

---

## Grammar

No changes.

---

## Task 1 — Add `eval_const_init` helper in `src/codegen.c`

Add a new static helper before the `codegen_statement` function that
recursively evaluates a parsed AST node as a compile-time integer constant.
On success it returns a `long`; on failure it calls `compile_error` with the
variable name for context.

```c
/* Evaluate a parsed initializer AST subtree as a compile-time integer
 * constant.  Mirrors eval_const_expr in the parser but operates on the
 * AST rather than the token stream.  Calls compile_error on any
 * non-constant node. */
static long eval_const_init(ASTNode *node, const char *varname) {
    if (node->type == AST_INT_LITERAL)
        return strtol(node->value, NULL, 10);
    if (node->type == AST_CHAR_LITERAL)
        return (long)(unsigned char)node->value[0];
    if (node->type == AST_SIZEOF_TYPE && node->full_type)
        return (long)type_size(node->full_type);
    if (node->type == AST_UNARY_OP && node->child_count == 1) {
        long v = eval_const_init(node->children[0], varname);
        if (strcmp(node->value, "-") == 0) return -v;
        if (strcmp(node->value, "~") == 0) return ~v;
        if (strcmp(node->value, "!") == 0) return !v;
        if (strcmp(node->value, "+") == 0) return v;
    }
    if (node->type == AST_BINARY_OP && node->child_count == 2) {
        long lhs = eval_const_init(node->children[0], varname);
        long rhs = eval_const_init(node->children[1], varname);
        if (strcmp(node->value, "+")  == 0) return lhs + rhs;
        if (strcmp(node->value, "-")  == 0) return lhs - rhs;
        if (strcmp(node->value, "*")  == 0) return lhs * rhs;
        if (strcmp(node->value, "/")  == 0) {
            if (rhs == 0)
                compile_error("error: division by zero in initializer for "
                              "static '%s'\n", varname);
            return lhs / rhs;
        }
        if (strcmp(node->value, "%")  == 0) {
            if (rhs == 0)
                compile_error("error: division by zero in initializer for "
                              "static '%s'\n", varname);
            return lhs % rhs;
        }
        if (strcmp(node->value, "<<") == 0) return lhs << rhs;
        if (strcmp(node->value, ">>") == 0) return lhs >> rhs;
        if (strcmp(node->value, "&")  == 0) return lhs & rhs;
        if (strcmp(node->value, "^")  == 0) return lhs ^ rhs;
        if (strcmp(node->value, "|")  == 0) return lhs | rhs;
    }
    compile_error(
        "error: initializer for block-scope static '%s' must be a "
        "constant expression\n", varname);
    return 0; /* unreachable */
}
```

**Notes:**

- `type_size` is declared in `include/type.h` and returns `t->size` (an
  `int`); no new include needed.
- The `strcmp` comparisons use the same operator-string values stored by the
  parser on `AST_BINARY_OP` and `AST_UNARY_OP` nodes.
- The unary `+` case (`+5`) normalizes to the same value — included for
  completeness even though it is unlikely in practice.
- Parenthesized sub-expressions are transparent in the AST (the parser does
  not emit a wrapper node for parentheses), so no special case is needed.

---

## Task 2 — Replace the ad-hoc check in `codegen_statement` with `eval_const_init`

In the SC_STATIC scalar arm (`codegen_statement`, lines ~4377–4399), replace
the existing 3-case check and its `else compile_error` block with a single
call to `eval_const_init`:

**Before:**

```c
/* Scalar static: validate that the initializer (if any) is a compile-time constant. */
long init_value = 0;
int is_initialized = 0;
if (node->child_count > 0) {
    ASTNode *init = node->children[0];
    if (init->type == AST_INT_LITERAL) {
        init_value = strtol(init->value, NULL, 10);
        is_initialized = 1;
    } else if (init->type == AST_CHAR_LITERAL) {
        init_value = (long)(unsigned char)init->value[0];
        is_initialized = 1;
    } else if (init->type == AST_UNARY_OP &&
               strcmp(init->value, "-") == 0 &&
               init->child_count > 0 &&
               init->children[0]->type == AST_INT_LITERAL) {
        init_value = -strtol(init->children[0]->value, NULL, 10);
        is_initialized = 1;
    } else {
        compile_error(
                "error: initializer for block-scope static '%s' must be a constant expression\n",
                node->value);
    }
}
```

**After:**

```c
/* Scalar static: evaluate the initializer as a compile-time constant.
 * Stage 103: eval_const_init handles the full expression tree. */
long init_value = 0;
int is_initialized = 0;
if (node->child_count > 0) {
    init_value = eval_const_init(node->children[0], node->value);
    is_initialized = 1;
}
```

`_Bool` normalization (any nonzero value becomes 1) is not needed here
because the existing codegen path for `LocalStaticVar` emission already
uses `data_init_directive(sv->kind)` and emits `sv->init_value` directly —
the value is stored as a `long` and emitted verbatim.  `_Bool` semantics are
enforced at assignment sites, not at static initialization.

---

## Task 3 — `src/version.c`

Bump `VERSION_STAGE` to `"01030000"`.

---

## Implementation order

1. `src/codegen.c` — add `eval_const_init` before `codegen_statement`.
2. `src/codegen.c` — replace the 3-case check with `eval_const_init` call.
3. `src/version.c` — bump stage.
4. Tests.
5. Documentation.

---

## Out of scope — do NOT do these in this stage

- **Pointer-typed block-scope static initializers** — `static int *p = NULL;`
  (already works via the existing literal check; non-null pointer constant
  expressions are out of scope).
- **Block-scope static array, struct, and union initializers** — those paths
  (`TYPE_ARRAY`/`TYPE_STRUCT`/`TYPE_UNION` in `codegen_statement`) have their
  own validation (must be `AST_INITIALIZER_LIST` or string literal) and are
  unchanged.
- **`sizeof(expr)` in constant initializers** — `sizeof(my_var)` in a static
  initializer produces `AST_SIZEOF_EXPR`; that node is not handled by
  `eval_const_init`.  Only `sizeof(type-name)` (`AST_SIZEOF_TYPE`) is
  supported, matching the restriction already in `eval_const_expr` (parser).
- **Relational, equality, and logical operators** (`==`, `!=`, `<`, `&&`,
  `||`, `?:`) — not in `eval_const_init`, matching the restriction in
  `eval_const_expr`.
- **`const`-qualified block-scope variables as constant references** — in C99,
  `const int x = 5; static int y = x;` is not legal because `x` is not an
  integer constant expression.  `eval_const_init` correctly rejects
  `AST_VAR_REF` nodes.
- **File-scope pointer and struct/union initializers** — already out of scope
  per stage 100.

---

## Bootstrap caveats

The compiler's own source uses block-scope `static` scalar initializers
extensively (counters, flags, indices) but all with simple literals (`0`, `1`,
`NULL`, `-1`).  All of these go through the `AST_INT_LITERAL` or
`AST_CHAR_LITERAL` branch of `eval_const_init`, which is identical in behavior
to the old code.  The self-host cycle should pass without changes to the
compiler's own source.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Valid tests

**Arithmetic expression:**
```c
int main(void) {
    static int x = 3 * 4 + 2;
    return x - 14;   /* expect 0 */
}
```

**Left shift:**
```c
int main(void) {
    static int page = 1 << 12;
    return page - 4096;   /* expect 0 */
}
```

**Bitwise OR:**
```c
int main(void) {
    static int mask = 0xF0 | 0x0F;
    return mask - 255;   /* expect 0 */
}
```

**Unary complement:**
```c
int main(void) {
    static int all = ~0;
    return (all & 0xFF) == 0xFF ? 0 : 1;   /* expect 0 */
}
```

**sizeof(type) in initializer:**
```c
int main(void) {
    static long sz = sizeof(long) * 8;
    return sz - 64;   /* expect 0 on LP64 */
}
```

**Persists across calls:**
```c
int count(void) {
    static int step = 2 * 3;   /* 6 */
    static int n = 0;
    n += step;
    return n;
}
int main(void) {
    count(); count();
    return count() - 18;   /* 6+6+6 = 18; expect 0 */
}
```

**Enum constant in expression:**
```c
enum { BASE = 10 };
int main(void) {
    static int limit = BASE * 2 + 5;
    return limit - 25;   /* expect 0 */
}
```

### Invalid tests

**Variable reference (not a constant):**
```c
int f(int n) {
    static int x = n + 1;
    return x;
}
int main(void) { return f(0); }
```
Expected error contains: `must be a constant expression`

**Function call (not a constant):**
```c
int g(void) { return 7; }
int main(void) {
    static int x = g();
    return x;
}
```
Expected error contains: `must be a constant expression`

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; extend the block-scope
  `static` bullet to note that scalar static initializers now accept the full
  constant-expression set (arithmetic, bitwise, shift, unary, `sizeof(type)`);
  refresh test totals.
- **`docs/grammar.md`** — no grammar changes.
- Run the **`update-supplemental-docs`** skill.
- **`docs/self-compilation-report.md`** — record the stage-103 self-host run.
