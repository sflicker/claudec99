# ClaudeC99 Stage 147 — Boolean / Logical Simplification

## Goal

Implement boolean and logical simplification rules in the stage-142 optimizer:
reduce `x && 0`, `x || 1`, `x && 1`, `x || 0`, and `!!x` to simpler forms when
one operand (or both, for the double-NOT case) is a compile-time constant.

This stage touches only `src/optimize.c`; no other source files change.  All
folding is gated behind `-O1`; the `-O0` path (the default) is unaffected.

---

## Background

The stage-142 optimizer walks the AST bottom-up in `optimize_expr`.  Prior
stages added the following rules relevant to logical operators:

- **Stage 143** — logical short-circuit folding: `0 && x → 0`, `nonzero || x → 1`,
  and both-literal cases for `&&` / `||`.
- **Stage 144** — constant unary folding: `!0 → 1`, `!nonzero_const → 0`, as
  well as two successive applications that already reduce `!!const` to 0 or 1.

This stage fills the remaining gaps: the right operand of `&&` / `||` is a
compile-time constant (stage 143 only checked the left operand), and `!!x` where
`x` is a non-constant expression.

Because the walk is bottom-up, the inner `!x` is visited before the outer `!`.
When `x` is a literal, stage-144 folds `!x` to 0 or 1, and then the outer `!`
folds that literal too — `!!const` is therefore already handled.  This stage
only needs to handle the case where `x` is a non-literal (i.e., the inner `!x`
could not be folded).

---

## Rule coverage

### Already handled by prior stages (do NOT re-implement)

| Pattern            | Result | Stage |
|--------------------|--------|-------|
| `!0`               | `1`    | 144   |
| `!nonzero_const`   | `0`    | 144   |
| `!!const`          | `0` or `1` | 144 (two applications) |
| `0 && x`           | `0`    | 143   |
| `nonzero && const` | `0` or `1` | 143 |
| `nonzero || x`     | `1`    | 143   |
| `0 || const`       | `0` or `1` | 143 |

### New rules added in this stage

#### Double logical NOT — `!!x` for non-constant `x`

| Pattern | Result        | Notes |
|---------|---------------|-------|
| `!!x`   | `(x != 0)`    | outer and inner `!` both `AST_UNARY_OP("!")`; inner child is not `AST_INT_LITERAL` |

The result `(x != 0)` is an `AST_BINARY_OP("!=")` node with `x` as the left
child and a new `AST_INT_LITERAL("0")` as the right child.  This is the
canonical "boolean normalization" (cast any truthy/falsy value to 0 or 1).

#### Boolean constant on the right of `&&` / `||`

| Pattern          | Result       | Notes |
|------------------|--------------|-------|
| `x && 0`         | `0`          | right operand is `AST_INT_LITERAL` with value `0` |
| `x \|\| nonzero` | `1`          | right operand is `AST_INT_LITERAL` with nonzero value |
| `x && nonzero`   | `(x != 0)`   | right operand is `AST_INT_LITERAL` with nonzero value; node mutated in place |
| `x \|\| 0`       | `(x != 0)`   | right operand is `AST_INT_LITERAL` with value `0`; operator replaced in place |

The spec lists `x && 1` and `x || 1` specifically; the implementation handles
any nonzero constant in the right position (generalises correctly because any
nonzero value is boolean-true in C).

---

## Side-effect note

**`x && 0 → 0` and `x || nonzero → 1`** drop the evaluation of `x`.  In
standard C, the left operand of `&&` / `||` is always evaluated, so these
transforms could change observable behaviour when `x` has side effects (e.g.,
`f() && 0`).

This optimizer takes the same aggressive position as stage 145 (`0 * x → 0`):
side effects of the discarded operand are not preserved.  This is consistent
behaviour within the project's optimizer, and is valid when the compiler is
known to operate on well-defined, effect-free expressions (e.g., constant
initialiser contexts or values the compiler itself has already determined to be
pure).

**`x && nonzero → (x != 0)` and `x || 0 → (x != 0)`** are exact semantic
equivalences: `x` is evaluated exactly once in both the original and the
rewritten form.  No side effects are dropped.

**`!!x → (x != 0)`** is also an exact equivalence: `x` is evaluated exactly
once.

---

## Task 1 — `src/optimize.c`: add `!!x` block in `optimize_expr`

Insert the following block **after** the "Constant unary folding" block (stage
144) and **before** the "Algebraic identity folding" block (stage 145).

```c
    /* Double logical NOT: !!x -> (x != 0) for non-constant x.
       When x is a literal, stage-144 already folds both ! applications;
       this rule fires only when the inner !x child is not an INT_LITERAL. */
    if (node->type == AST_UNARY_OP &&
            node->child_count == 1 &&
            strcmp(node->value, "!") == 0) {
        ASTNode *inner = node->children[0];
        ASTNode *x;
        ASTNode *neq;
        ASTNode *zero;
        int fire;

        fire = (inner != NULL &&
                inner->type == AST_UNARY_OP &&
                inner->child_count == 1 &&
                strcmp(inner->value, "!") == 0 &&
                inner->children[0] != NULL &&
                inner->children[0]->type != AST_INT_LITERAL);

        if (fire) {
            x = inner->children[0];
            inner->children[0] = NULL; /* prevent double-free of x */
            ast_free(node);            /* frees outer ! and inner ! (not x) */
            neq = ast_new(AST_BINARY_OP, util_strdup("!="));
            neq->decl_type = TYPE_INT;
            zero = ast_new(AST_INT_LITERAL, "0");
            zero->decl_type = TYPE_INT;
            ast_add_child(neq, x);
            ast_add_child(neq, zero);
            return neq;
        }
    }
```

---

## Task 2 — `src/optimize.c`: add binary boolean block in `optimize_expr`

Insert the following block **after** the strength reduction block (stage 146)
and **before** the final `return node;`.

```c
    /* Boolean/logical simplification: x&&0, x||nonzero, x&&nonzero, x||0.
       Cases where the LEFT operand is a literal are already handled by the
       logical short-circuit block (stage 143); this block handles a nonzero
       or zero RIGHT constant. */
    if (node->type == AST_BINARY_OP && node->child_count == 2) {
        const char *op       = node->value;
        ASTNode    *right    = node->children[1];
        int right_is_lit     = (right->type == AST_INT_LITERAL);
        long rval            = right_is_lit ? strtol(right->value, NULL, 0) : 0L;
        int right_is_zero    = right_is_lit && (rval == 0L);
        int right_is_nonzero = right_is_lit && (rval != 0L);
        ASTNode *z;
        ASTNode *zero_lit;

        /* x && 0 -> 0 */
        if (strcmp(op, "&&") == 0 && right_is_zero) {
            z = ast_new(AST_INT_LITERAL, "0");
            z->decl_type = TYPE_INT;
            ast_free(node);
            return z;
        }

        /* x || nonzero -> 1 */
        if (strcmp(op, "||") == 0 && right_is_nonzero) {
            z = ast_new(AST_INT_LITERAL, "1");
            z->decl_type = TYPE_INT;
            ast_free(node);
            return z;
        }

        /* x && nonzero -> (x != 0): replace right child with 0, change op */
        if (strcmp(op, "&&") == 0 && right_is_nonzero) {
            zero_lit = ast_new(AST_INT_LITERAL, "0");
            zero_lit->decl_type = TYPE_INT;
            ast_free(right);
            node->children[1] = zero_lit;
            node->value = util_strdup("!=");
            node->decl_type = TYPE_INT;
            return node;
        }

        /* x || 0 -> (x != 0): right child is already 0, just change operator */
        if (strcmp(op, "||") == 0 && right_is_zero) {
            node->value = util_strdup("!=");
            node->decl_type = TYPE_INT;
            return node;
        }
    }
```

No other changes to `src/optimize.c` are needed.  `ast_add_child`, `util_strdup`,
`strtol`, `strcmp` are all already available.

---

## Memory management

### `!!x` block (Task 1)

- `inner = node->children[0]` borrows the pointer; the slot is NOT nulled before
  `ast_free(node)`.
- `inner->children[0] = NULL` is set first to detach `x` from the inner `!`
  before the free traversal reaches it.
- `ast_free(node)` recurses: frees the outer `!` node → recurses into
  `node->children[0]` (inner `!`) → frees inner `!` node → recurses into
  `inner->children[0]` which is now `NULL` → stops.  `x` is NOT freed.
- A fresh `AST_BINARY_OP("!=")` is built with `ast_new` + `ast_add_child`.
  `ast_add_child` dynamically grows the children array.

### Binary boolean block (Task 2)

**`x && 0 → 0` and `x || nonzero → 1`:**
- `ast_free(node)` frees the entire subtree (both children including `x` and
  the literal).
- A fresh `AST_INT_LITERAL` is returned.

**`x && nonzero → (x != 0)`:**
- `ast_free(right)` frees only the nonzero literal (right child).
- `node->children[1]` is replaced with a new `AST_INT_LITERAL("0")`.
- `node->value` is replaced with `util_strdup("!=")`.  The old `"&&"` string
  is not explicitly freed (minor leak, consistent with stages 145–146).
- The left child `x` and the outer node itself are retained.

**`x || 0 → (x != 0)`:**
- Neither child is freed: the right child (`AST_INT_LITERAL("0")`) becomes the
  `0` operand of `!=` directly.
- `node->value` is replaced with `util_strdup("!=")`.

---

## Result type convention

- **`x && 0 → 0` and `x || nonzero → 1`:** new literals carry `decl_type =
  TYPE_INT`, `is_unsigned = 0`.
- **`x && nonzero → (x != 0)` and `x || 0 → (x != 0)`:** the mutated node
  gets `decl_type = TYPE_INT`.  The left child retains its original type; the
  new zero literal is `TYPE_INT`.  This matches how the parser types `!=`
  results.
- **`!!x → (x != 0)`:** the new `AST_BINARY_OP("!=")` carries `decl_type =
  TYPE_INT`.  The transferred `x` node retains its original type and
  `is_unsigned` flag.

---

## Out of scope — do NOT do these in this stage

- `0 || x → x` — not in the stage 147 spec; a future stage.
- `nonzero && x → x` — not in spec; a future stage.
- `x && x → x`, `x || x → x` — structural equality beyond `VAR_REF` — future.
- `!(x != 0) → (x == 0)` — negation distribution — future.
- Negation folding: `-(-x) → x` — a separate checklist item (stage 148+).
- Conditional expression folding (`AST_CONDITIONAL_EXPR`) — future.
- Dead-branch elimination — future.
- Float literal boolean rules — future.
- Overflow detection or UB warnings.

---

## Bootstrap caveats

`src/optimize.c` must continue to compile cleanly under the C0 compiler
(previous stage binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations must appear at the top of a block before any executable
  statements.
  - Task 1 block: `inner`, `x`, `neq`, `zero`, `fire` are all declared at the
    top of the outer `if` block.  The `if (fire)` body uses only assignments
    and function calls — no new declarations.
  - Task 2 block: `op`, `right`, `right_is_lit`, `rval`, `right_is_zero`,
    `right_is_nonzero`, `z`, `zero_lit` are all declared at the top of the
    outer `if` block.
- `ast_add_child` is declared in `"ast.h"`, already included.
- `util_strdup` is in `"util.h"`, already included.
- `strtol` is in `<stdlib.h>`, already included.
- `strcmp` is in `<string.h>`, already included.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
are unaffected.

### test_bool_simplify_and_zero

```c
/* test/integration/test_bool_simplify_and_zero/test_bool_simplify_and_zero.c */
#include <stdio.h>
int main(void) {
    int x = 7;
    int a = x && 0;
    int b = 0 && 0;    /* left=0: handled by stage-143 */
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `0 0`

### test_bool_simplify_or_nonzero

```c
/* test/integration/test_bool_simplify_or_nonzero/test_bool_simplify_or_nonzero.c */
#include <stdio.h>
int main(void) {
    int x = 0;
    int a = x || 1;
    int b = x || 5;    /* any nonzero right operand */
    int c = 1 || 1;    /* left=1: handled by stage-143 */
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1 1 1`

### test_bool_simplify_and_one

Verify that `x && nonzero → (x != 0)` produces correct boolean values:

```c
/* test/integration/test_bool_simplify_and_one/test_bool_simplify_and_one.c */
#include <stdio.h>
int main(void) {
    int x = 5;
    int y = 0;
    int a = x && 1;    /* x is truthy  -> 1 */
    int b = y && 1;    /* y is falsy   -> 0 */
    int c = x && 3;    /* any nonzero  -> 1 */
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1 0 1`

### test_bool_simplify_or_zero

Verify that `x || 0 → (x != 0)` produces correct boolean values:

```c
/* test/integration/test_bool_simplify_or_zero/test_bool_simplify_or_zero.c */
#include <stdio.h>
int main(void) {
    int x = 5;
    int y = 0;
    int a = x || 0;    /* x is truthy  -> 1 */
    int b = y || 0;    /* y is falsy   -> 0 */
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1 0`

### test_bool_double_not

```c
/* test/integration/test_bool_double_not/test_bool_double_not.c */
#include <stdio.h>
int main(void) {
    int x = 5;
    int y = 0;
    int z = -3;
    int a = !!x;       /* non-zero var  -> 1 */
    int b = !!y;       /* zero var      -> 0 */
    int c = !!z;       /* negative var  -> 1 */
    int d = !!0;       /* const: stage-144 handles -> 0 */
    int e = !!7;       /* const: stage-144 handles -> 1 */
    printf("%d %d %d %d %d\n", a, b, c, d, e);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1 0 1 0 1`

### test_bool_simplify_combined

Verify that boolean rules compose with each other and with prior folding rules
in a single bottom-up pass:

```c
/* test/integration/test_bool_simplify_combined/test_bool_simplify_combined.c */
#include <stdio.h>
int main(void) {
    int x = 4;
    /* !!x -> (x != 0); then (x != 0) && 1 -> ((x != 0) != 0) */
    int a = !!x && 1;
    /* (x || 0) && 0 -> (x != 0) && 0 -> 0 */
    int b = (x || 0) && 0;
    /* !!x || 0 -> (x != 0) || 0 -> (x != 0) != 0 */
    int c = !!x || 0;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1 0 1`

### Regression

Run the full test suite at both `-O0` and `-O1` via `./run_tests.sh`.  All
existing tests must continue to pass.

---

## Implementation order

1. In `src/optimize.c`, locate the end of the "Constant unary folding" block
   (after the `}` that closes the `if (do_fold)` block), just before the
   "Algebraic identity folding" block.
2. Insert the Task 1 (`!!x`) block.
3. Locate the end of the strength reduction block (just before the final
   `return node;`).
4. Insert the Task 2 binary boolean block.
5. Build: `cmake --build build`.
6. Smoke test:
   ```sh
   echo '#include <stdio.h>
   int main(void) {
       int x = 5; int y = 0;
       printf("%d %d %d %d\n", !!x, x && 0, x && 1, x || 0);
       return 0; }' > /tmp/bl.c
   ./build/ccompiler -O1 /tmp/bl.c -o /tmp/bl.asm
   nasm -f elf64 /tmp/bl.asm -o /tmp/bl.o && gcc /tmp/bl.o -o /tmp/bl && /tmp/bl
   ```
   Expected output: `1 0 1 1`
7. Add integration tests.
8. Run `./run_tests.sh` — all tests pass.
9. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
10. Bump `VERSION_STAGE` to `"01470000"` in `src/version.c`.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Boolean / logical simplification"
  item and all five sub-bullets as complete (`[x]`), annotating Stage 147.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-147 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01470000"`).
