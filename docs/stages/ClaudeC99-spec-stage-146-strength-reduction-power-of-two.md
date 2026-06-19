# ClaudeC99 Stage 146 — Strength Reduction: Power-of-Two Multiply/Divide

## Goal

Implement strength reduction in the stage-142 optimizer: replace multiplication
by a compile-time power-of-two constant with a left shift, and replace division
by a compile-time power-of-two constant (unsigned dividend or signed with
non-negative value known statically) with a right shift.

This stage touches only `src/optimize.c`; no other source files change.  All
folding is gated behind `-O1`; the `-O0` path (the default) is unaffected.

---

## Background

The stage-142 optimizer walks the AST bottom-up in `optimize_expr`.  Stage 145
added algebraic identity rules.  This stage adds a new rule block after the
algebraic identity block that fires when the right operand of `*` or `/` is an
`AST_INT_LITERAL` that is an exact power of two.

Because the walk is bottom-up, by the time we inspect an `AST_BINARY_OP` the
right operand has already been simplified.  A power-of-two literal that emerges
from a prior constant fold (e.g., `1 << 3` folded to `8`) is therefore eligible
for strength reduction in the same pass.

A positive integer `v` is a power of two iff `v > 0 && (v & (v - 1)) == 0`.
The shift amount is `N` such that `v == (1 << N)`, computed as the number of
trailing zeros in `v`.

---

## Rule coverage

### Multiply by power of two

| Pattern       | Result    | Notes |
|---------------|-----------|-------|
| `x * 2^N`     | `x << N`  | right operand is `AST_INT_LITERAL` with value that is a power of two; N ≥ 1 |
| `2^N * x`     | `x << N`  | left operand is `AST_INT_LITERAL`; commutative form |

The `* 1` (`2^0`) case is already handled by the stage-145 multiplicative
identity rule and will never reach this block.

### Divide by power of two (unsigned or statically non-negative signed)

| Pattern       | Result    | Notes |
|---------------|-----------|-------|
| `x / 2^N`     | `x >> N`  | right operand is a power of two; `x` is `unsigned` or explicitly non-negative (see scope section) |

Division by 1 (`2^0`) is already handled by stage-145.

---

## Scope of the division rule

Replacing `x / 2^N` with `x >> N` is always valid for **unsigned** dividends
(logical right shift).  For signed integers, arithmetic right shift (`x >> N`)
rounds toward negative infinity, whereas C signed division rounds toward zero,
so the substitution is only valid when the dividend is known to be non-negative
at compile time.

This stage recognizes the dividend as statically non-negative when:

1. `left->is_unsigned` is true (unsigned type), **or**
2. `left->type == AST_INT_LITERAL` and `strtol(left->value, NULL, 0) >= 0`.

Case 2 handles constant-folded dividends that are still positive integers
(e.g., `6 / 2` where `6` is an `AST_INT_LITERAL`; constant binary folding from
stage 143 would already have reduced this to `3`, but the rule is listed for
completeness).  In practice the case that fires most often is case 1 (unsigned
variables).

Signed variables whose runtime value happens to be non-negative are **out of
scope** for this stage.  The strength reduction does not apply to them.

---

## Helper: `is_power_of_two` and `log2_of`

Two small helpers are declared as `static` functions (or inline expressions)
inside the new rule block.  To keep the C0-compatibility constraint (declarations
before statements in every block), compute both values from `rval`:

```c
    long  rval        = strtol(right->value, NULL, 0);
    int   is_pow2     = (rval > 0L) && ((rval & (rval - 1L)) == 0L);
    int   shift_amt   = 0;
    long  tmp;
```

Compute `shift_amt` with a loop at the top of the block:

```c
    tmp = rval;
    while (tmp > 1L) { tmp >>= 1; shift_amt++; }
```

This runs at compile time (the right operand is a literal) and adds no runtime
cost.

---

## Task 1 — `src/optimize.c`: add strength reduction block in `optimize_expr`

Insert the following block **after** the "Algebraic identity folding" block
(i.e., after `if (z != NULL) { ast_free(node); return z; }`) and **before** the
final `return node;`.

```c
    /* Strength reduction: x * 2^N → x << N; x / 2^N → x >> N (unsigned). */
    if (node->type == AST_BINARY_OP && node->child_count == 2) {
        const char *op      = node->value;
        ASTNode    *left    = node->children[0];
        ASTNode    *right   = node->children[1];
        int  do_mul         = (strcmp(op, "*") == 0);
        int  do_div         = (strcmp(op, "/") == 0);
        int  right_is_lit   = (right->type == AST_INT_LITERAL);
        int  left_is_lit    = (left->type  == AST_INT_LITERAL);
        long rval           = right_is_lit ? strtol(right->value, NULL, 0) : 0L;
        long lval           = left_is_lit  ? strtol(left->value,  NULL, 0) : 0L;
        int  r_is_pow2      = right_is_lit && (rval > 1L) && ((rval & (rval - 1L)) == 0L);
        int  l_is_pow2      = left_is_lit  && (lval > 1L) && ((lval & (lval - 1L)) == 0L);
        int  r_shift        = 0;
        int  l_shift        = 0;
        int  left_nonneg    = left_is_lit  && (lval >= 0L);
        long tmp;
        ASTNode *lit;
        char buf[16];

        if (r_is_pow2) {
            tmp = rval;
            while (tmp > 1L) { tmp >>= 1; r_shift++; }
        }
        if (l_is_pow2) {
            tmp = lval;
            while (tmp > 1L) { tmp >>= 1; l_shift++; }
        }

        /* x * 2^N → x << N  (right operand is power of two) */
        if (do_mul && r_is_pow2) {
            snprintf(buf, sizeof(buf), "%d", r_shift);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type = TYPE_INT;
            lit->is_unsigned = 0;
            ast_free(right);
            node->children[1] = lit;
            node->value = util_strdup("<<");
            return node;
        }

        /* 2^N * x → x << N  (left operand is power of two; move x to left slot) */
        if (do_mul && l_is_pow2) {
            snprintf(buf, sizeof(buf), "%d", l_shift);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type = TYPE_INT;
            lit->is_unsigned = 0;
            node->children[0] = right;   /* x moves to left slot  */
            node->children[1] = lit;
            ast_free(left);              /* free the 2^N literal  */
            node->value = util_strdup("<<");
            return node;
        }

        /* x / 2^N → x >> N  (unsigned dividend or statically non-negative literal) */
        if (do_div && r_is_pow2 && (left->is_unsigned || left_nonneg)) {
            snprintf(buf, sizeof(buf), "%d", r_shift);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type = TYPE_INT;
            lit->is_unsigned = 0;
            ast_free(right);
            node->children[1] = lit;
            node->value = util_strdup(">>");
            return node;
        }
    }
```

No other changes to `src/optimize.c` are needed — all required `#include`
directives and utilities (`util_strdup`, `strtol`, `snprintf`, `strcmp`) were
already added in stages 143–145.

---

## Memory management

All three rules mutate the existing `AST_BINARY_OP` node in place and return
it.  The node itself and its left child are never freed by these rules.

**`x * 2^N` and `x / 2^N` (right operand is the power-of-two):**

- `ast_free(right)` frees the power-of-two literal.
- A new shift-amount `AST_INT_LITERAL` is stored in `node->children[1]`.
- `node->value` is replaced with the new operator string.

**`2^N * x` (left operand is the power-of-two):**

- `node->children[0]` is set to `right` (the `x` operand) before freeing.
- `ast_free(left)` frees the power-of-two literal (still referenced by the
  local variable `left`, not by `node->children[0]` at that point).
- A new shift-amount literal is stored in `node->children[1]`.
- `node->value` is replaced with `util_strdup("<<")`.

---

## Result type convention

The mutated node retains the `decl_type` and `is_unsigned` of the original
`AST_BINARY_OP` node, which the parser set from the operand types.  The new
shift-amount literal is always `TYPE_INT` / not unsigned — consistent with how
the parser produces shift-operand literals.

---

## Out of scope — do NOT do these in this stage

- Division by power-of-two for signed dividends whose non-negativity is not
  statically known — a later stage (may require value-range analysis).
- Modulo by power-of-two (`x % 2^N → x & (2^N - 1)`) — a later stage.
- Strength reduction on floating-point multiply/divide — a later stage.
- Chained shift collapsing (`x << 2 << 3 → x << 5`) — a later stage.
- Boolean simplification, negation folding, conditional folding — later stages.

---

## Bootstrap caveats

`src/optimize.c` must continue to compile cleanly under the C0 compiler
(previous stage binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations must appear at the top of a block before any statements.  All
  new locals (`op`, `left`, `right`, `do_mul`, `do_div`, `right_is_lit`,
  `left_is_lit`, `rval`, `lval`, `r_is_pow2`, `l_is_pow2`, `r_shift`,
  `l_shift`, `left_nonneg`, `tmp`, `lit`, `buf`) are declared at the top of
  the outer `if` block before any executable statements, so no inner `if`
  branch introduces a new declaration.
- `util_strdup` is available in C0 (added in stage 83).
- `snprintf` is in `<stdio.h>`, already included.
- `strtol` is in `<stdlib.h>`, already included.
- `strcmp` is in `<string.h>`, already included.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
are unaffected.

### test_strength_reduce_mul_pow2

```c
/* test/integration/test_strength_reduce_mul_pow2/test_strength_reduce_mul_pow2.c */
#include <stdio.h>
int main(void) {
    int x = 3;
    int a = x * 2;    /* x << 1 */
    int b = x * 4;    /* x << 2 */
    int c = x * 8;    /* x << 3 */
    int d = x * 16;   /* x << 4 */
    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `6 12 24 48`

### test_strength_reduce_mul_pow2_commutative

```c
/* test/integration/test_strength_reduce_mul_pow2_commutative/test_strength_reduce_mul_pow2_commutative.c */
#include <stdio.h>
int main(void) {
    int x = 5;
    int a = 2 * x;    /* x << 1 */
    int b = 8 * x;    /* x << 3 */
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `10 40`

### test_strength_reduce_div_pow2_unsigned

```c
/* test/integration/test_strength_reduce_div_pow2_unsigned/test_strength_reduce_div_pow2_unsigned.c */
#include <stdio.h>
int main(void) {
    unsigned int x = 48;
    unsigned int a = x / 2;    /* x >> 1 */
    unsigned int b = x / 4;    /* x >> 2 */
    unsigned int c = x / 8;    /* x >> 3 */
    printf("%u %u %u\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `24 12 6`

### test_strength_reduce_no_signed_div

Verify that signed division by a power of two is **not** reduced (the dividend
is a signed variable with unknown sign at compile time):

```c
/* test/integration/test_strength_reduce_no_signed_div/test_strength_reduce_no_signed_div.c */
#include <stdio.h>
int main(void) {
    int x = -7;
    int a = x / 2;    /* must NOT become x >> 1; C rounds toward zero, >> rounds down */
    printf("%d\n", a);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `-3`

### test_strength_reduce_combined

Verify that strength reduction composes with constant folding in a single
bottom-up pass:

```c
/* test/integration/test_strength_reduce_combined/test_strength_reduce_combined.c */
#include <stdio.h>
int main(void) {
    int x = 7;
    /* (1 << 3) folds to 8 by stage-143; then x * 8 reduces to x << 3 */
    int a = x * (1 << 3);
    /* (2 + 2) folds to 4; then x * 4 reduces to x << 2 */
    int b = x * (2 + 2);
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `56 28`

### Regression

Run the full test suite at both `-O0` and `-O1` via `./run_tests.sh`.  All
existing tests must continue to pass.

---

## Implementation order

1. In `src/optimize.c`, locate the end of `optimize_expr` (just before the
   final `return node;`, after the algebraic identity block).
2. Insert the strength reduction block.
3. Build: `cmake --build build`.
4. Smoke test:
   ```sh
   echo '#include <stdio.h>
   int main(void) { int x = 3; unsigned int y = 48; printf("%d %u\n", x * 8, y / 4); return 0; }' \
     > /tmp/sr.c
   ./build/ccompiler -O1 /tmp/sr.c -o /tmp/sr.asm
   nasm -f elf64 /tmp/sr.asm -o /tmp/sr.o && gcc /tmp/sr.o -o /tmp/sr && /tmp/sr
   ```
   Expected output: `24 12`
5. Add integration tests.
6. Run `./run_tests.sh` — all tests pass.
7. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
8. Bump `VERSION_STAGE` to `"01460000"` in `src/version.c`.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Strength reduction on
  multiplications by powers of two" item and both sub-bullets as complete
  (`[x]`), annotating Stage 146.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-146 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01460000"`).
