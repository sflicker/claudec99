# ClaudeC99 Stage 145 — Algebraic Identity Folding

## Goal

Implement algebraic identity rules in the stage-142 optimizer: simplify
`AST_BINARY_OP` expressions where one or both operands are constant integers or
structurally identical variable references, even when the other operand is a
non-constant expression.

This stage touches only `src/optimize.c`; no other source files change.  All
folding is gated behind `-O1`; the `-O0` path (the default) is unaffected.

---

## Background

The stage-142 optimizer walks the AST bottom-up in `optimize_expr`.  Stages 143
and 144 added constant folding: both operands must be `AST_INT_LITERAL`.  This
stage extends the optimizer with rules that fire when only **one** operand is a
constant, or when both operands are the **same variable reference**.

Because the walk is bottom-up, by the time we inspect an `AST_BINARY_OP` its
children have already been simplified.  If a child emerged as `AST_INT_LITERAL`
from a prior constant fold, the algebraic rules below fire on that result.

---

## Rule coverage

### Additive identities

| Pattern  | Result | Notes |
|----------|--------|-------|
| `x + 0`  | `x`    | right operand is `AST_INT_LITERAL` with value `0` |
| `0 + x`  | `x`    | left operand is `AST_INT_LITERAL` with value `0` |
| `x - 0`  | `x`    | right operand is `AST_INT_LITERAL` with value `0` |

### Multiplicative identities

| Pattern  | Result | Notes |
|----------|--------|-------|
| `x * 1`  | `x`    | right operand is `AST_INT_LITERAL` with value `1` |
| `1 * x`  | `x`    | left operand is `AST_INT_LITERAL` with value `1` |
| `x / 1`  | `x`    | right operand is `AST_INT_LITERAL` with value `1` |

### Zero propagation

| Pattern  | Result | Notes |
|----------|--------|-------|
| `x * 0`  | `0`    | right operand is `AST_INT_LITERAL` with value `0` |
| `0 * x`  | `0`    | left operand is `AST_INT_LITERAL` with value `0` |
| `0 / x`  | `0`    | left operand is `AST_INT_LITERAL` with value `0`; see UB note below |

### Self-cancellation and bitwise zero

| Pattern  | Result | Notes |
|----------|--------|-------|
| `x - x`  | `0`    | both children are `AST_VAR_REF` with same `value` string |
| `x ^ x`  | `0`    | both children are `AST_VAR_REF` with same `value` string |
| `x & 0`  | `0`    | right operand is `AST_INT_LITERAL` with value `0` |
| `0 & x`  | `0`    | left operand is `AST_INT_LITERAL` with value `0` |
| `x \| 0` | `x`    | right operand is `AST_INT_LITERAL` with value `0` |
| `0 \| x` | `x`    | left operand is `AST_INT_LITERAL` with value `0` |

### Identity masks

| Pattern    | Result | Notes |
|------------|--------|-------|
| `x & ~0`   | `x`    | `~0` is folded to `-1L` by stage-144 unary folding before this rule fires |
| `~0 & x`   | `x`    | commutative form |

`~0` arrives as `AST_INT_LITERAL` with value `"-1"` (the result of stage-144
constant unary folding).  The rule detects `strtol(value, NULL, 0) == -1L`.

---

## UB note — `0 / x`

When `x` is zero at runtime, `0 / x` is undefined behavior regardless of the
source form.  The optimizer is entitled to assume UB does not occur; folding
`0 / x → 0` is therefore valid.  No warning is emitted in this stage.

---

## Self-cancellation scope

`x - x → 0` and `x ^ x → 0` are recognized only when both children are
`AST_VAR_REF` nodes with the same `value` field (variable name).  Expressions
with side effects (`f() - f()`, `*p - *p`) are intentionally **not** folded:
the structural check here is deliberately shallow.

---

## Task 1 — `src/optimize.c`: add algebraic identity block in `optimize_expr`

Insert the following block after the "Constant unary folding" block (the last
rule block in `optimize_expr`) and before the final `return node;`.

```c
    /* Algebraic identity folding. */
    if (node->type == AST_BINARY_OP && node->child_count == 2) {
        const char *op        = node->value;
        ASTNode    *left      = node->children[0];
        ASTNode    *right     = node->children[1];
        int left_is_lit       = (left->type  == AST_INT_LITERAL);
        int right_is_lit      = (right->type == AST_INT_LITERAL);
        long lval             = left_is_lit  ? strtol(left->value,  NULL, 0) : 0L;
        long rval             = right_is_lit ? strtol(right->value, NULL, 0) : 0L;
        int left_is_zero      = left_is_lit  && (lval == 0L);
        int right_is_zero     = right_is_lit && (rval == 0L);
        int left_is_one       = left_is_lit  && (lval == 1L);
        int right_is_one      = right_is_lit && (rval == 1L);
        int left_is_allones   = left_is_lit  && (lval == -1L);
        int right_is_allones  = right_is_lit && (rval == -1L);
        int both_same_var     = (left->type  == AST_VAR_REF &&
                                 right->type == AST_VAR_REF &&
                                 strcmp(left->value, right->value) == 0);
        ASTNode *z;

        /* Identity rules: result is one existing child.
           Null the kept child's slot before ast_free to avoid double-free. */
        if (strcmp(op, "+") == 0 && right_is_zero)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "+") == 0 && left_is_zero)
            { node->children[1] = NULL; ast_free(node); return right; }
        if (strcmp(op, "-") == 0 && right_is_zero)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "*") == 0 && right_is_one)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "*") == 0 && left_is_one)
            { node->children[1] = NULL; ast_free(node); return right; }
        if (strcmp(op, "/") == 0 && right_is_one)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "|") == 0 && right_is_zero)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "|") == 0 && left_is_zero)
            { node->children[1] = NULL; ast_free(node); return right; }
        if (strcmp(op, "&") == 0 && right_is_allones)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "&") == 0 && left_is_allones)
            { node->children[1] = NULL; ast_free(node); return right; }

        /* Zero rules: free entire subtree and return a fresh zero literal. */
        z = NULL;
        if      (strcmp(op, "*") == 0 && right_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = left->decl_type;  z->is_unsigned = left->is_unsigned;  }
        else if (strcmp(op, "*") == 0 && left_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = right->decl_type; z->is_unsigned = right->is_unsigned; }
        else if (strcmp(op, "/") == 0 && left_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = right->decl_type; z->is_unsigned = right->is_unsigned; }
        else if (strcmp(op, "&") == 0 && right_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = left->decl_type;  z->is_unsigned = left->is_unsigned;  }
        else if (strcmp(op, "&") == 0 && left_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = right->decl_type; z->is_unsigned = right->is_unsigned; }
        else if (strcmp(op, "-") == 0 && both_same_var)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = left->decl_type;  z->is_unsigned = left->is_unsigned;  }
        else if (strcmp(op, "^") == 0 && both_same_var)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = left->decl_type;  z->is_unsigned = left->is_unsigned;  }
        if (z != NULL) { ast_free(node); return z; }
    }
```

No other changes to `src/optimize.c` are needed — all required `#include`
directives were already added in stage 143.

---

## Memory management

**Identity rules (returning an existing child):**

Before calling `ast_free(node)`, set the kept child's slot to `NULL`:

```c
node->children[0] = NULL;   /* keep left; prevent ast_free from recursing into it */
ast_free(node);             /* frees the parent node, the right subtree, and the children array */
return left;
```

`ast_free` recurses into all non-NULL children.  Nulling the kept slot prevents
it from freeing the subtree we are returning.

**Zero rules (returning a new literal):**

`ast_free(node)` frees the entire subtree (both children).  The new literal is
allocated with `ast_new(AST_INT_LITERAL, "0")`.  The string `"0"` is a string
constant and is **not** freed by `ast_free` (which does not touch `node->value`).

---

## Result type convention

**Identity rules:** The returned node is the actual child node, which already
carries the correct `decl_type` and `is_unsigned` from parsing.  No type field
needs to be set.

**Zero rules:** The new zero literal inherits `decl_type` and `is_unsigned` from
the **left** operand for symmetric operators (`*`, `&`) and from whichever operand
determines the result for asymmetric cases (`0 / x`, `0 * x` — from right operand
`x`).  This approximates the usual arithmetic conversions without requiring full
type-ranking logic.  A future stage may refine this.

---

## Out of scope — do NOT do these in this stage

- Strength reduction: `x * 2^N → x << N` — a later stage.
- Boolean simplification: `x && 0`, `x || 1` with non-constant `x` — a later stage.
- Negation folding: `--x → x`, `!!x` chain collapse — a later stage.
- Conditional expression folding: `AST_CONDITIONAL_EXPR` with constant condition — a later stage.
- Dead-branch elimination in `if`/`while`/`for` — a later stage.
- Deep structural equality for `x - x` / `x ^ x` beyond a single `AST_VAR_REF` — a later stage.
- Float literal algebraic rules — a later stage.
- Overflow detection or UB warnings.

---

## Bootstrap caveats

`src/optimize.c` must continue to compile cleanly under the C0 compiler
(previous stage binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations must appear at the top of a block before any statements.  All
  new locals (`op`, `left`, `right`, `left_is_lit`, `right_is_lit`, `lval`,
  `rval`, `left_is_zero`, `right_is_zero`, `left_is_one`, `right_is_one`,
  `left_is_allones`, `right_is_allones`, `both_same_var`, `z`) are declared
  at the top of the outer `if` block before any executable statements.  The
  compound statements `{ node->children[0] = NULL; ... return left; }` do not
  declare new variables, so they are legal statements inside the outer block.
- `strtol` is in `<stdlib.h>`, already included from stage 143.
- `strcmp` is in `<string.h>`, already included from stage 143.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
are unaffected.

### test_fold_additive_identity

```c
/* test/integration/test_fold_additive_identity/test_fold_additive_identity.c */
#include <stdio.h>
int main(void) {
    int x = 42;
    int a = x + 0;
    int b = 0 + x;
    int c = x - 0;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `42 42 42`

### test_fold_multiplicative_identity

```c
/* test/integration/test_fold_multiplicative_identity/test_fold_multiplicative_identity.c */
#include <stdio.h>
int main(void) {
    int x = 7;
    int a = x * 1;
    int b = 1 * x;
    int c = x / 1;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `7 7 7`

### test_fold_zero_propagation

```c
/* test/integration/test_fold_zero_propagation/test_fold_zero_propagation.c */
#include <stdio.h>
int main(void) {
    int x = 99;
    int a = x * 0;
    int b = 0 * x;
    int c = 0 / x;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `0 0 0`

### test_fold_self_cancellation

```c
/* test/integration/test_fold_self_cancellation/test_fold_self_cancellation.c */
#include <stdio.h>
int main(void) {
    int x = 5;
    int a = x - x;
    int b = x ^ x;
    int c = x & 0;
    int d = x | 0;
    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `0 0 0 5`

### test_fold_identity_mask

```c
/* test/integration/test_fold_identity_mask/test_fold_identity_mask.c */
#include <stdio.h>
int main(void) {
    int x = 0xAB;
    int a = x & ~0;
    printf("%d\n", a);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `171`

Note: `~0` is folded to `-1` by the stage-144 unary folding pass first, then
`x & -1` is recognized as the all-ones mask identity.

### test_fold_algebraic_combined

Verify that algebraic rules compose correctly with each other and with constant
folding in a single bottom-up pass:

```c
/* test/integration/test_fold_algebraic_combined/test_fold_algebraic_combined.c */
#include <stdio.h>
int main(void) {
    int x = 3;
    int a = (x + 0) * 1;         /* x + 0 → x; x * 1 → x */
    int b = (x * 0) + (x * 1);   /* x * 0 → 0; x * 1 → x; 0 + x → x */
    int c = (x - x) | (1 * x);   /* x - x → 0; 1 * x → x; 0 | x → x */
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `3 3 3`

### Regression

Run the full test suite at both `-O0` and `-O1` via `./run_tests.sh`.  All
existing tests must continue to pass.

---

## Implementation order

1. In `src/optimize.c`, locate the end of `optimize_expr` (just before the
   final `return node;`).
2. Insert the algebraic identity block after the existing "Constant unary
   folding" block.
3. Build: `cmake --build build`.
4. Smoke test:
   ```sh
   echo '#include <stdio.h>
   int main(void) { int x = 5; printf("%d %d\n", x + 0, x * 0); return 0; }' \
     > /tmp/alg.c
   ./build/ccompiler -O1 /tmp/alg.c -o /tmp/alg.asm
   nasm -f elf64 /tmp/alg.asm -o /tmp/alg.o && gcc /tmp/alg.o -o /tmp/alg && /tmp/alg
   ```
   Expected output: `5 0`
5. Add integration tests.
6. Run `./run_tests.sh` — all tests pass.
7. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
8. Bump `VERSION_STAGE` to `"01450000"` in `src/version.c`.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Algebraic identities" item and
  all five sub-bullets as complete (`[x]`), annotating Stage 145.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-145 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01450000"`).
