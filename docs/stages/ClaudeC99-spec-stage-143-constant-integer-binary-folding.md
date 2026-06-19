# ClaudeC99 Stage 143 — Constant Integer Binary Folding

## Goal

Implement the first real optimization rule in the stage-142 optimizer infrastructure:
**constant integer binary folding**.  When both children of an `AST_BINARY_OP`
node are `AST_INT_LITERAL`, compute the result at compile time and replace the
entire binary node with a single `AST_INT_LITERAL`.

Also folded in this stage: the unary bitwise-NOT operator `~` on a constant
operand (listed under the "bitwise" bullet of the checklist item), handled as
a separate rule inside `optimize_expr` for `AST_UNARY_OP`.

This stage touches only `src/optimize.c`; no other source files change.  All
folding is gated behind `-O1`; the `-O0` path (the default) is unaffected.

---

## Background

The stage-142 optimizer walks the AST bottom-up in `optimize_expr`.  After all
children of a node have been recursively visited, the parent sees the
(already-optimized) children.  This bottom-up order is exactly right for constant
folding: by the time we inspect an `AST_BINARY_OP`, its two children have already
been simplified as much as possible, so if both emerged as `AST_INT_LITERAL` the
fold is safe.

Integer literal values are stored as decimal strings in `node->value` and are read
back with `strtol(node->value, NULL, 0)`.  The `decl_type` field carries the C
type (`TYPE_INT`, `TYPE_LONG`, etc.) and `is_unsigned` marks unsigned variants.

---

## Operator coverage

### Arithmetic binary (both children `AST_INT_LITERAL`)

| Operator | `node->value` | Notes |
|----------|--------------|-------|
| `+`      | `"+"`        | |
| `-`      | `"-"`        | |
| `*`      | `"*"`        | |
| `/`      | `"/"`        | skip fold when right operand is 0 |
| `%`      | `"%"`        | skip fold when right operand is 0 |

### Bitwise binary (both children `AST_INT_LITERAL`)

| Operator | `node->value` |
|----------|--------------|
| `&`      | `"&"`        |
| `\|`     | `"\|"`       |
| `^`      | `"^"`        |
| `<<`     | `"<<"`       |
| `>>`     | `">>"`       |

### Relational binary (both children `AST_INT_LITERAL`) — result is 0 or 1

| Operator | `node->value` |
|----------|--------------|
| `<`      | `"<"`        |
| `<=`     | `"<="`       |
| `>`      | `">"`        |
| `>=`     | `">="`       |
| `==`     | `"=="`       |
| `!=`     | `"!="`       |

### Logical binary — short-circuit rules (see below)

| Operator | `node->value` |
|----------|--------------|
| `&&`     | `"&&"`       |
| `\|\|`   | `"\|\|"`     |

### Unary bitwise-NOT (one child `AST_INT_LITERAL`)

| Operator | `node->value` | Node type |
|----------|--------------|-----------|
| `~`      | `"~"`        | `AST_UNARY_OP` |

---

## Task 1 — `src/optimize.c`: binary folding rule in `optimize_expr`

Insert the following rule block **after** the generic child-recursion loop and
**before** the final `return node;` in `optimize_expr`.  All existing code
(the child-recursion loop and the final `return`) remains unchanged around it.

```c
    /* Constant integer binary folding. */
    if (node->type == AST_BINARY_OP &&
        node->child_count == 2 &&
        node->children[0]->type == AST_INT_LITERAL &&
        node->children[1]->type == AST_INT_LITERAL) {

        const char *op  = node->value;
        long lval = strtol(node->children[0]->value, NULL, 0);
        long rval = strtol(node->children[1]->value, NULL, 0);
        long result = 0;
        int  do_fold = 1;
        int  result_is_bool = 0; /* relational/logical → TYPE_INT, 0 or 1 */

        if (strcmp(op, "+")  == 0) { result = lval + rval; }
        else if (strcmp(op, "-")  == 0) { result = lval - rval; }
        else if (strcmp(op, "*")  == 0) { result = lval * rval; }
        else if (strcmp(op, "/")  == 0) {
            if (rval == 0) { do_fold = 0; }
            else           { result = lval / rval; }
        }
        else if (strcmp(op, "%")  == 0) {
            if (rval == 0) { do_fold = 0; }
            else           { result = lval % rval; }
        }
        else if (strcmp(op, "&")  == 0) { result = lval & rval; }
        else if (strcmp(op, "|")  == 0) { result = lval | rval; }
        else if (strcmp(op, "^")  == 0) { result = lval ^ rval; }
        else if (strcmp(op, "<<") == 0) { result = lval << rval; }
        else if (strcmp(op, ">>") == 0) { result = lval >> rval; }
        else if (strcmp(op, "<")  == 0) { result = lval < rval;  result_is_bool = 1; }
        else if (strcmp(op, "<=") == 0) { result = lval <= rval; result_is_bool = 1; }
        else if (strcmp(op, ">")  == 0) { result = lval > rval;  result_is_bool = 1; }
        else if (strcmp(op, ">=") == 0) { result = lval >= rval; result_is_bool = 1; }
        else if (strcmp(op, "==") == 0) { result = lval == rval; result_is_bool = 1; }
        else if (strcmp(op, "!=") == 0) { result = lval != rval; result_is_bool = 1; }
        else { do_fold = 0; } /* logical && / || handled separately below */

        if (do_fold) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%ld", result);
            {
                ASTNode *lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
                lit->decl_type = result_is_bool
                                 ? TYPE_INT
                                 : node->children[0]->decl_type;
                lit->is_unsigned = result_is_bool
                                   ? 0
                                   : node->children[0]->is_unsigned;
                ast_free(node);
                return lit;
            }
        }
    }

    /* Logical short-circuit folding. */
    if (node->type == AST_BINARY_OP && node->child_count == 2) {
        const char *op = node->value;
        int left_is_lit  = (node->children[0]->type == AST_INT_LITERAL);
        int right_is_lit = (node->children[1]->type == AST_INT_LITERAL);

        if (strcmp(op, "&&") == 0 && left_is_lit) {
            long lval = strtol(node->children[0]->value, NULL, 0);
            if (lval == 0) {
                /* 0 && anything → 0 (short-circuit, right not evaluated) */
                ASTNode *zero = ast_new(AST_INT_LITERAL, "0");
                zero->decl_type = TYPE_INT;
                ast_free(node);
                return zero;
            }
            if (right_is_lit) {
                long rval = strtol(node->children[1]->value, NULL, 0);
                ASTNode *lit = ast_new(AST_INT_LITERAL, rval != 0 ? "1" : "0");
                lit->decl_type = TYPE_INT;
                ast_free(node);
                return lit;
            }
        }

        if (strcmp(op, "||") == 0 && left_is_lit) {
            long lval = strtol(node->children[0]->value, NULL, 0);
            if (lval != 0) {
                /* nonzero || anything → 1 (short-circuit, right not evaluated) */
                ASTNode *one = ast_new(AST_INT_LITERAL, "1");
                one->decl_type = TYPE_INT;
                ast_free(node);
                return one;
            }
            if (right_is_lit) {
                long rval = strtol(node->children[1]->value, NULL, 0);
                ASTNode *lit = ast_new(AST_INT_LITERAL, rval != 0 ? "1" : "0");
                lit->decl_type = TYPE_INT;
                ast_free(node);
                return lit;
            }
        }
    }
```

### Result type convention

- **Relational and logical results** (`result_is_bool == 1`): `decl_type = TYPE_INT`,
  `is_unsigned = 0`.  These always produce 0 or 1, which is an `int` in C.
- **Arithmetic/bitwise results**: inherit `decl_type` and `is_unsigned` from the
  **left operand**.  This approximates the usual arithmetic conversions without
  pulling in the full type-ranking logic.  It is conservative: if the left operand
  is `TYPE_LONG`, the result is `TYPE_LONG`; if `TYPE_INT`, the result is `TYPE_INT`.
  A future stage may refine this.

### Memory note

`util_strdup(buf)` (from `include/util.h`) allocates a heap copy of the formatted
string.  The `ast_new()` function stores the pointer directly without copying, so
the heap allocation must outlive the node.  The string literals `"0"` and `"1"`
used for logical results are string constants and do **not** need to be freed.

---

## Task 2 — `src/optimize.c`: unary bitwise-NOT rule in `optimize_expr`

Insert the following rule block in the same location (after child-recursion,
before the final `return node;`), adjacent to the binary-folding block:

```c
    /* Constant unary bitwise-NOT folding. */
    if (node->type == AST_UNARY_OP &&
        node->child_count == 1 &&
        strcmp(node->value, "~") == 0 &&
        node->children[0]->type == AST_INT_LITERAL) {

        long val = strtol(node->children[0]->value, NULL, 0);
        long result = ~val;
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", result);
        {
            ASTNode *lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type  = node->children[0]->decl_type;
            lit->is_unsigned = node->children[0]->is_unsigned;
            ast_free(node);
            return lit;
        }
    }
```

---

## Task 3 — add required includes to `src/optimize.c`

Add the following `#include` directives at the top of `src/optimize.c` alongside
the existing ones:

```c
#include <stdio.h>   /* snprintf */
#include <string.h>  /* strcmp   */
#include "util.h"    /* util_strdup */
```

The existing file already includes `<stddef.h>`, `"optimize.h"`, and `"ast.h"`.

---

## Division-by-zero guard

When the operator is `/` or `%` and `rval == 0`, set `do_fold = 0` and leave
the binary node in place.  This lets codegen emit the operation as-is, which
will trap or produce undefined behavior at runtime — the same semantics as
un-optimized code.  We do **not** emit a warning in this stage; a future stage
may add a "division by constant zero" diagnostic.

---

## Short-circuit semantics

For `&&` and `||`, C guarantees the right operand is not evaluated when the
result can be determined from the left.  The optimizer must preserve this:

- `0 && X` → fold to `0` even when `X` has side-effects (but since `X` is an
  `AST_INT_LITERAL`, it has none; the rule is still correct).
- `nonzero || X` → fold to `1` for the same reason.
- When the left is constant-nonzero for `&&` or constant-zero for `||`, the
  result depends on the right.  We only fold if the right is also a constant.

This matches the checklist annotation: "second operand only folded when first is constant."

---

## Out of scope — do NOT do these in this stage

- Unary `-`, `+`, `!` folding — stage 144 (constant unary folding checklist item).
- Algebraic identities (`x + 0`, `x * 1`, etc.) — a later stage.
- Strength reduction (multiply by power of two → shift) — a later stage.
- Conditional expression folding (`cond ? a : b` with constant `cond`) — a later stage.
- Dead-branch elimination in `if`/`while`/`for` — a later stage.
- Nested / cascaded folding beyond what the bottom-up pass naturally provides
  (e.g., `(1 + 2) * (3 + 4)` will fold in two passes through the recursive
  walk, which is correct).
- Overflow detection or UB warnings for folded arithmetic.
- Float literal folding — arithmetic on `AST_FLOAT_LITERAL` nodes — a later stage.

---

## Bootstrap caveats

`src/optimize.c` must continue to compile cleanly under the C0 compiler
(previous stage binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations must appear at the top of a block before any statements.  The
  nested block `{ ASTNode *lit = ...; ... }` in the fold rules satisfies this.
- `util_strdup` is available in C0 (it was added in stage 83).
- `snprintf` is in `<stdio.h>`, which is already a permitted include.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file so that the
optimizer pass runs.  Existing tests (no `.cflags` or with other flags) are
unaffected.

### test_fold_arithmetic

```c
/* test/integration/test_fold_arithmetic/test_fold_arithmetic.c */
#include <stdio.h>
int main(void) {
    int a = 3 + 4;
    int b = 10 - 3;
    int c = 6 * 7;
    int d = 22 / 7;
    int e = 22 % 7;
    printf("%d %d %d %d %d\n", a, b, c, d, e);
    return 0;
}
```

`.cflags`: `-O1`  
`.expected`: `7 7 42 3 1`

### test_fold_bitwise

```c
/* test/integration/test_fold_bitwise/test_fold_bitwise.c */
#include <stdio.h>
int main(void) {
    int a = 0xF0 & 0xFF;
    int b = 0x0F | 0xF0;
    int c = 0xFF ^ 0x0F;
    int d = 1 << 5;
    int e = 128 >> 2;
    int f = ~0;
    printf("%d %d %d %d %d %d\n", a, b, c, d, e, f);
    return 0;
}
```

`.cflags`: `-O1`  
`.expected`: `240 255 240 32 32 -1`

### test_fold_relational

```c
/* test/integration/test_fold_relational/test_fold_relational.c */
#include <stdio.h>
int main(void) {
    int lt = 3 < 5;
    int le = 5 <= 5;
    int gt = 7 > 3;
    int ge = 3 >= 4;
    int eq = 4 == 4;
    int ne = 4 != 5;
    printf("%d %d %d %d %d %d\n", lt, le, gt, ge, eq, ne);
    return 0;
}
```

`.cflags`: `-O1`  
`.expected`: `1 1 1 0 1 1`

### test_fold_logical

```c
/* test/integration/test_fold_logical/test_fold_logical.c */
#include <stdio.h>
int main(void) {
    int a = 1 && 1;
    int b = 0 && 1;
    int c = 1 && 0;
    int d = 0 && 0;
    int e = 1 || 0;
    int f = 0 || 1;
    int g = 0 || 0;
    printf("%d %d %d %d %d %d %d\n", a, b, c, d, e, f, g);
    return 0;
}
```

`.cflags`: `-O1`  
`.expected`: `1 0 0 0 1 1 0`

### test_fold_divzero_skipped

Division by constant zero must **not** crash the compiler — the binary node
must be left in place, and codegen emits the divide instruction which will
trap at runtime.

```c
/* test/integration/test_fold_divzero_skipped/test_fold_divzero_skipped.c */
int main(void) {
    return 0;
}
```

This test only verifies that `ccompiler -O1` exits 0 on valid code with no
divide-by-zero literals.  A separate manual smoke-test verifies that
`ccompiler -O1` on a file containing `int x = 1 / 0;` does not crash the
compiler (it may crash at runtime, which is acceptable).

`.cflags`: `-O1`  
`.expected`: *(empty — exit code 0)*

### test_fold_nested

Verify that nested constant expressions fold fully in one bottom-up pass:

```c
/* test/integration/test_fold_nested/test_fold_nested.c */
#include <stdio.h>
int main(void) {
    int x = (1 + 2) * (3 + 4);   /* → 3 * 7 → 21 */
    int y = 1 << (2 + 1);         /* → 1 << 3 → 8  */
    printf("%d %d\n", x, y);
    return 0;
}
```

`.cflags`: `-O1`  
`.expected`: `21 8`

### Regression

Run the full test suite at both `-O0` and `-O1` via `./run_tests.sh`.  All
existing tests must continue to pass (the fold produces the same runtime
values as unfolded code).

---

## Implementation order

1. Add `#include <stdio.h>`, `#include <string.h>`, `#include "util.h"` to
   `src/optimize.c` if not already present.
2. Insert the binary-folding rule block into `optimize_expr`.
3. Insert the unary-`~` rule block into `optimize_expr`.
4. Build: `cmake --build build`.
5. Smoke test: `./build/ccompiler -O1 test/integration/test_fold_arithmetic/test_fold_arithmetic.c -o /tmp/fold.asm && nasm -f elf64 /tmp/fold.asm -o /tmp/fold.o && gcc /tmp/fold.o -o /tmp/fold && /tmp/fold`.
6. Add integration tests.
7. Run `./run_tests.sh` — all tests pass.
8. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
9. Bump `VERSION_STAGE` to `"01430000"` in `src/version.c`.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Constant integer binary folding"
  item and its four sub-bullets as complete (`[x]`).  Also mark the `~` (unary)
  sub-bullet under that item.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-143 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01430000"`).
