# ClaudeC99 Stage 152 — Constant Propagation for `const` Scalar Locals

## Goal

Implement constant propagation for `const`-qualified scalar local variables
initialized with a compile-time integer literal.  At each `AST_VAR_REF` of
such a variable, substitute the recorded literal in `optimize_expr`, making
the constant value visible to subsequent folding passes (stages 143–151).

This stage touches only `src/optimize.c`; no other source files change.  All
propagation is gated behind `-O1`; the `-O0` default path is unaffected.

---

## Background

In C99, a `const`-qualified local variable with an integer-literal initializer
can never be modified after its declaration.  The value is therefore a
compile-time constant, but the optimizer currently cannot see it:

```c
const int N = 8;
int a = N * 4;          /* N not literal; stage 143 cannot fold N * 4 */
if (N > 10) { ... }     /* condition not literal; stage 150 cannot eliminate */
```

After this stage, `optimize_expr` replaces any `AST_VAR_REF` whose name
matches a recorded `const`-scalar-literal declaration with a fresh
`AST_INT_LITERAL`.  Because the optimizer is a bottom-up tree walk, the
substituted literal is immediately visible to all folding rules that check
the parent node (binary folding, dead-branch elimination, etc.).

The approach uses a file-static per-function table (`g_const_table`) that
maps variable names to recorded literal values.  Scope nesting is handled
in the `AST_BLOCK` case of `optimize_stmt` by saving and restoring the
table's current length.

Related prior work:

- **Stage 39** — `is_const` field on `ASTNode`; assignment to const-qualified
  lvalue rejected at codegen.
- **Stage 142** — optimizer skeleton: `optimize_expr` / `optimize_stmt` /
  `optimize_translation_unit`.
- **Stage 143** — constant integer binary folding; depends on both operands
  being `AST_INT_LITERAL`.
- **Stage 150** — dead-branch elimination; depends on the condition being
  `AST_INT_LITERAL`.
- **Stage 151** — sizeof constant folding; demonstrated the pattern of
  substituting a complex node with a fresh `AST_INT_LITERAL`.

---

## Eligibility rules

A declaration is eligible for propagation if and only if all of the following
hold at the point in `optimize_stmt` where the `AST_DECLARATION` is processed:

| Criterion | How checked |
|-----------|-------------|
| `const`-qualified | `node->is_const == 1` |
| Scalar integer type | `is_scalar_int_type(node->decl_type)` returns 1 (see helper below) |
| Has an initializer that resolved to a literal | `node->child_count >= 1 && node->children[0]->type == AST_INT_LITERAL` |

The third check runs **after** `optimize_expr` has been called on the
initializer.  This means that `const int x = 3 * 4;` is eligible: stage 143
folds `3 * 4` to `12` before the check runs, so `children[0]` is
`AST_INT_LITERAL "12"`.

**Not eligible** (do not propagate):

- `const` aggregates — structs, unions, arrays (`decl_type` is
  `TYPE_STRUCT`, `TYPE_UNION`, or `TYPE_ARRAY`).
- `const` pointers — `decl_type` is `TYPE_POINTER`.
- `const` floating-point scalars — `decl_type` is `TYPE_FLOAT` or
  `TYPE_DOUBLE`.
- Declarations without initializers (`child_count == 0`) — the value is
  indeterminate and the compiler already rejects this for `const` locals at
  codegen, but be safe.
- Global `const` variables — these never appear inside a function body's
  `AST_BLOCK`, so the table is never populated with them.  `AST_VAR_REF`
  nodes for globals remain unsubstituted.

---

## Already handled by prior stages (do NOT re-implement)

| Pattern                      | Result          | Stage |
|------------------------------|-----------------|-------|
| `const_a OP const_b`         | folded literal  | 143   |
| `OP const`                   | folded literal  | 144   |
| `x && 0`, `0 && x`           | `0`             | 147   |
| `x \|\| 1`, `1 \|\| x`       | `1`             | 147   |
| `-(-x)`                      | `x`             | 148   |
| `const ? T : F`              | selected branch | 149   |
| `if/while/for (0)`           | dead-branch drop| 150   |
| `sizeof(type/expr)`          | folded literal  | 151   |

---

## Semantic notes

**Result type** — The substituted `AST_INT_LITERAL` carries `decl_type` and
`is_unsigned` from the `AST_DECLARATION` node (the declared type of the
variable), not from the initializer literal.  For example, `const unsigned
int x = 5;` produces a literal with `decl_type = TYPE_INT, is_unsigned = 1`.
This is correct because the C type of the expression `x` is the type of the
variable, not the type of its initializer.

**Scope** — In C99, a `const int x = 5;` inside an inner block does not
affect references to `x` in the enclosing block.  The propagation table is
scope-safe: `AST_BLOCK` saves `g_const_count` before processing children
and restores it after, so entries added in an inner scope are invisible
outside it.  If an inner `const int x` shadows an outer one, the inner
entry is added at a higher index and found first (scan is right-to-left).

**Reassignment** — A `const` local can never be the left-hand side of an
assignment; codegen already rejects this at stage 39.  The optimizer
therefore assumes a `const` entry in `g_const_table` remains valid for the
entire scope in which it was declared.

**`ast_free` does not free `node->value`** — Confirmed by reading `src/ast.c`:
`ast_free` frees `node->children` and `node` itself but not `node->value`.
When a `AST_VAR_REF` is freed and replaced with a fresh `AST_INT_LITERAL`,
the `name` pointer stored in `g_const_table` (which aliases the declaration's
`value` string) remains valid because the declaration node is not freed.

---

## Task — `src/optimize.c`: add const propagation

### 1 — Static table and helpers

Insert the following **before** `optimize_expr`.

**Table definition:**

```c
/* Const propagation table for the current function.
   Populated by AST_DECLARATION (is_const + scalar-int + literal init).
   Entries are scoped: AST_BLOCK saves/restores g_const_count on entry/exit.
   Reset to 0 before each function in optimize_translation_unit. */
#define CONST_PROP_MAX 64
typedef struct {
    const char *name;        /* aliases AST_DECLARATION.value; stable lifetime */
    long        value;       /* numeric value of the recorded initializer */
    TypeKind    decl_type;   /* declared type of the variable */
    int         is_unsigned; /* 1 if the variable is unsigned */
} ConstEntry;
static ConstEntry g_const_table[CONST_PROP_MAX];
static int        g_const_count = 0;
```

**Helper `is_scalar_int_type`:**

```c
/* Return 1 if k is a scalar integer type eligible for const propagation. */
static int is_scalar_int_type(TypeKind k) {
    switch (k) {
    case TYPE_BOOL:
    case TYPE_CHAR:
    case TYPE_SHORT:
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_LONG_LONG:
    case TYPE_UNSIGNED_LONG_LONG:
        return 1;
    default:
        return 0;
    }
}
```

**Helper `const_prop_lookup`:**

```c
/* Scan g_const_table right-to-left (innermost scope first).
   Returns the ConstEntry for name, or NULL if not found. */
static const ConstEntry *const_prop_lookup(const char *name) {
    int j;
    for (j = g_const_count - 1; j >= 0; j--) {
        if (strcmp(name, g_const_table[j].name) == 0)
            return &g_const_table[j];
    }
    return NULL;
}
```

### 2 — `AST_VAR_REF` substitution in `optimize_expr`

Add the following block immediately **before** the final `return node;` in
`optimize_expr` (after the `AST_SIZEOF_EXPR` block):

```c
    /* Const propagation: replace a reference to a const-scalar-literal
       local with the recorded integer literal so folding passes can see it. */
    if (node->type == AST_VAR_REF && node->value != NULL) {
        const ConstEntry *ce = const_prop_lookup(node->value);
        if (ce != NULL) {
            char buf[32];
            ASTNode *lit;
            snprintf(buf, sizeof(buf), "%ld", ce->value);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type   = ce->decl_type;
            lit->is_unsigned = ce->is_unsigned;
            ast_free(node);  /* frees VAR_REF node; value string not freed by ast_free */
            return lit;
        }
    }
```

### 3 — `AST_DECLARATION` recording in `optimize_stmt`

Split the combined `AST_DECLARATION` / `AST_DECL_LIST` case in `optimize_stmt`
into two separate cases.  Replace:

```c
    case AST_DECLARATION:
    case AST_DECL_LIST:
        /* Initializer expressions are children of DECLARATION nodes. */
        for (i = 0; i < node->child_count; i++)
            node->children[i] = optimize_expr(node->children[i]);
        return node;
```

With:

```c
    case AST_DECLARATION:
        /* Optimize initializer expression (child 0, if present). */
        if (node->child_count > 0)
            node->children[0] = optimize_expr(node->children[0]);
        /* Record const scalar-int locals whose initializer folded to a literal. */
        if (node->is_const &&
                is_scalar_int_type(node->decl_type) &&
                node->child_count > 0 &&
                node->children[0] != NULL &&
                node->children[0]->type == AST_INT_LITERAL &&
                g_const_count < CONST_PROP_MAX) {
            g_const_table[g_const_count].name        = node->value;
            g_const_table[g_const_count].value       = strtol(node->children[0]->value, NULL, 0);
            g_const_table[g_const_count].decl_type   = node->decl_type;
            g_const_table[g_const_count].is_unsigned = node->is_unsigned;
            g_const_count++;
        }
        return node;

    case AST_DECL_LIST:
        /* Multi-variable declarations: optimize each sub-declaration's
           initializer.  Const-prop recording for multi-var decls is out
           of scope for this stage. */
        for (i = 0; i < node->child_count; i++)
            node->children[i] = optimize_expr(node->children[i]);
        return node;
```

### 4 — Scope save/restore in `AST_BLOCK`

Replace the existing `AST_BLOCK` case:

```c
    case AST_BLOCK:
        for (i = 0; i < node->child_count; i++)
            node->children[i] = optimize_stmt(node->children[i]);
        return node;
```

With:

```c
    case AST_BLOCK: {
        /* Save g_const_count so entries added in this block are invisible
           in the enclosing scope after the block exits. */
        int saved_count = g_const_count;
        for (i = 0; i < node->child_count; i++)
            node->children[i] = optimize_stmt(node->children[i]);
        g_const_count = saved_count;
        return node;
    }
```

### 5 — Reset table per function in `optimize_translation_unit`

In `optimize_translation_unit`, reset `g_const_count` to 0 **before** calling
`optimize_stmt` on each function body:

```c
ASTNode *optimize_translation_unit(ASTNode *root, int opt_level) {
    int i;
    if (opt_level == 0) return root;

    for (i = 0; i < root->child_count; i++) {
        ASTNode *decl = root->children[i];
        if (decl->type == AST_FUNCTION_DECL) {
            int last = decl->child_count - 1;
            if (last >= 0 && decl->children[last]->type == AST_BLOCK) {
                g_const_count = 0; /* reset per-function const table */
                decl->children[last] = optimize_stmt(decl->children[last]);
            }
        }
    }
    return root;
}
```

### 6 — Update file-top comment block

Add to the existing comment at the top of `optimize.c`:

```
 * Stage 152: const propagation -- const scalar locals with literal init
 *            replaced at each AST_VAR_REF with the recorded literal.
```

---

## Memory management

**Recording** — The `g_const_table` stores `node->value` (a `const char *`)
directly without copying.  This is safe because the `AST_DECLARATION` node
remains in the AST for the lifetime of the function optimization pass; `ast_free`
does not free `node->value`.

**Substitution** — When a `AST_VAR_REF` is substituted:
1. `snprintf(buf, ...)` formats the recorded `long` into a stack buffer.
2. `util_strdup(buf)` copies it to a heap string (since `ast_free` does not free
   `value`, this string lives until program exit; acceptable for a compiler).
3. `ast_new(AST_INT_LITERAL, ...)` allocates a fresh node.
4. `ast_free(node)` frees the `AST_VAR_REF` node (not its `value`).
5. The fresh `AST_INT_LITERAL` is returned.

No aliasing issue exists between the freed `AST_VAR_REF` and the returned
literal.

---

## Bootstrap caveats

`src/optimize.c` must compile cleanly under the C0 compiler (previous-stage
binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations (`buf`, `lit`, `ce`, `saved_count`) must appear at the top of
  each block before any executable statements.
- The `typedef struct { ... } ConstEntry;` must appear at file scope
  (before any function definition), not inside a function.
- `strcmp` is in `<string.h>`, already included via `util.h`.
- `strtol` is in `<stdlib.h>`, already included.
- `snprintf` is in `<stdio.h>`, already included.
- `util_strdup` is declared in `"util.h"`, already included.
- `ast_new`, `ast_free`, `AST_INT_LITERAL`, `AST_VAR_REF`, `AST_DECLARATION`,
  `AST_DECL_LIST`, `AST_BLOCK`, `TYPE_BOOL`, `TYPE_CHAR`, `TYPE_SHORT`,
  `TYPE_INT`, `TYPE_LONG`, `TYPE_LONG_LONG`, `TYPE_UNSIGNED_LONG_LONG` are
  declared in `"ast.h"`, already included.
- The macro `CONST_PROP_MAX 64` and the `ConstEntry` typedef must be placed
  before both the table variables and any function that references them.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
(including all `-O0` tests) are unaffected.

### test_const_prop_basic

Direct use of a `const` local — value substituted and used verbatim:

```c
/* test/integration/test_const_prop_basic/test_const_prop_basic.c */
#include <stdio.h>
int main(void) {
    const int x = 42;
    const long y = 100;
    printf("%d %ld\n", x, y);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `42 100`

### test_const_prop_fold

`const` variable in a binary expression — substitution exposes the literal
to stage-143 binary folding:

```c
/* test/integration/test_const_prop_fold/test_const_prop_fold.c */
#include <stdio.h>
int main(void) {
    const int N = 8;
    int a = N * 4;        /* -> 8 * 4 -> 32 */
    int b = N - 3;        /* -> 8 - 3 -> 5  */
    int c = N + N;        /* -> 8 + 8 -> 16 */
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `32 5 16`

### test_const_prop_dead_branch

`const` variable in a condition — substitution exposes the literal to
stage-150 dead-branch elimination:

```c
/* test/integration/test_const_prop_dead_branch/test_const_prop_dead_branch.c */
#include <stdio.h>
int main(void) {
    const int FLAG = 0;
    int result = 0;
    if (FLAG) { result = 99; } else { result = 10; }
    const int LIMIT = 5;
    int n = 0;
    while (LIMIT - 5) { n = 99; }    /* LIMIT-5 -> 0; stage 150 drops loop */
    printf("%d %d\n", result, n);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `10 0`

### test_const_prop_scope

Inner `const` declaration shadows outer one — scope restore ensures the
outer value is used after the inner block exits:

```c
/* test/integration/test_const_prop_scope/test_const_prop_scope.c */
#include <stdio.h>
int main(void) {
    const int x = 5;
    int a = x;            /* -> 5 */
    {
        const int x = 10;
        int b = x;        /* -> 10 (inner x) */
        printf("%d\n", b);
    }
    int c = x;            /* -> 5 (outer x restored after inner block) */
    printf("%d %d\n", a, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `10`
`5 5`

### test_const_prop_init_fold

`const` variable initialized with an expression that folds to a literal —
the initializer is folded by stage 143 first, then recorded:

```c
/* test/integration/test_const_prop_init_fold/test_const_prop_init_fold.c */
#include <stdio.h>
int main(void) {
    const int x = 3 * 4;    /* folds to 12 before recording */
    int a = x + 1;          /* -> 12 + 1 -> 13 */
    printf("%d\n", a);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `13`

### Regression

Run the full test suite via `./run_tests.sh`.  All existing tests must
continue to pass at both `-O0` and `-O1`.  In particular:

- Non-`const` variables must NOT be substituted.
- `const` aggregate variables (struct, array) must NOT be substituted.
- Variables referenced before their declaration in the source must not
  appear in the table (C99 forbids this; the parser/codegen already reject
  it, so this is not an optimizer concern, but confirm no crash).

---

## Implementation order

1. In `src/optimize.c`, add the `CONST_PROP_MAX` macro, the `ConstEntry`
   typedef, the `g_const_table` and `g_const_count` file-static variables,
   the `is_scalar_int_type` helper, and the `const_prop_lookup` helper —
   all before the definition of `optimize_expr`.
2. In `optimize_expr`, add the `AST_VAR_REF` substitution block immediately
   before the final `return node;`.
3. In `optimize_stmt`, split the combined `AST_DECLARATION` / `AST_DECL_LIST`
   case as described above and add the recording logic to `AST_DECLARATION`.
4. In `optimize_stmt`, add scope save/restore to the `AST_BLOCK` case.
5. In `optimize_translation_unit`, add `g_const_count = 0;` before each
   function-body optimization.
6. Update the file-top comment block.
7. Build: `cmake --build build`.
8. Smoke test:
   ```sh
   printf '#include <stdio.h>\nint main(void){\n  const int N=8;\n  printf("%%d\\n", N*4);\n  return 0;}\n' > /tmp/cprop.c
   ./build/ccompiler -O1 /tmp/cprop.c -o /tmp/cprop.asm
   nasm -f elf64 /tmp/cprop.asm -o /tmp/cprop.o && gcc /tmp/cprop.o -o /tmp/cprop && /tmp/cprop
   ```
   Expected output: `32`
9. Add the five integration tests.
10. Run `./run_tests.sh` — all tests pass.
11. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
12. Bump `VERSION_STAGE` to `"01520000"` in `src/version.c`.

---

## Out of scope — do NOT do these in this stage

- **Global `const` variables** — not visible inside function bodies as
  `AST_DECLARATION` nodes in `optimize_stmt`; require symbol-table access
  (codegen's `find_global`) that the optimizer does not have.
- **`const` in `AST_DECL_LIST`** (multi-variable declarations, e.g.,
  `const int a = 1, b = 2;`) — the current optimizer iterates `DECL_LIST`
  children via `optimize_expr`, which does not call the recording logic.
  Defer to a follow-up stage if needed.
- **`const` aggregates / pointers** — initializers are not integer literals;
  `is_scalar_int_type` rejects them.
- **`const` floating-point scalars** — `TYPE_FLOAT` / `TYPE_DOUBLE` are not
  in `is_scalar_int_type`; float literals are `AST_FLOAT_LITERAL`, not
  `AST_INT_LITERAL`.
- **`static const` locals** — technically safe to propagate (the value is
  invariant), but `static` storage can require address-taking in some patterns.
  Defer to a later stage after evaluating any codegen interactions.
- **`extern const` locals** — cannot appear in standard C99; not a concern.
- **Propagation across function calls** — inter-procedural constant
  propagation requires a call graph; out of scope for this AST-level pass.
- **Enum constants (`AST_VAR_REF` for enum members)** — enum members are
  folded at parse time to `AST_INT_LITERAL` by stage 99; `AST_VAR_REF`
  nodes for enum names do not appear in the AST.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Constant propagation for
  simple `const`-qualified scalar locals" item as complete (`[x]`),
  annotating Stage 152; add a `## Stage 152` section after `## Stage 151`.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-152 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01520000"`).
