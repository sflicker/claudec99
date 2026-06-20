# ClaudeC99 Stage 148 — Negation Folding

## Goal

Implement double-unary-minus folding in the stage-142 optimizer: reduce
`-(-x)` to `x` for non-constant expressions `x`.

This stage touches only `src/optimize.c`; no other source files change.  All
folding is gated behind `-O1`; the `-O0` path (the default) is unaffected.

---

## Background

The stage-142 optimizer walks the AST bottom-up in `optimize_expr`.  Prior
stages added the following rules relevant to unary chains:

- **Stage 144** — constant unary folding: `-const → -const_literal`,
  `+const → const`, `!const → 0_or_1`, `~const → bitwise_complement`.
  Because the walk is bottom-up, two successive applications already reduce
  `-(-const)` to the original value: the inner `-const` folds first, producing
  a new `AST_INT_LITERAL`, and then the outer `-` folds that literal.
- **Stage 147** — double logical NOT: `!!x → (x != 0)` for non-constant `x`.

This stage handles the arithmetic analogue: `-(-x)` for non-constant `x`.
When `x` is a compile-time integer literal the two-pass constant folding
already collapses the double negation, so this rule fires only when the inner
child is not an `AST_INT_LITERAL`.

The checklist item "Negation folding" also listed `!!x` double-not chain
collapse; that half was completed in Stage 147.  This stage closes the
remaining half: double arithmetic negation.

---

## Rule coverage

### Already handled by prior stages (do NOT re-implement)

| Pattern      | Result | Stage |
|--------------|--------|-------|
| `-const`     | folded literal | 144 |
| `-(-const)`  | folded literal (two passes) | 144 |
| `!!x`        | `(x != 0)` | 147 |

### New rule added in this stage

#### Double arithmetic negation — `-(-x)` for non-constant `x`

| Pattern   | Result | Notes |
|-----------|--------|-------|
| `-(-x)`   | `x`    | outer and inner operands both `AST_UNARY_OP("-")`; innermost child is not `AST_INT_LITERAL` |

The result is `x` itself — the inner `AST_UNARY_OP("-")` node and the outer
`AST_UNARY_OP("-")` wrapper are both freed, and the grandchild `x` is returned
directly.

---

## Semantic note

`-(-x)` is an exact semantic equivalence to `x`: no side effects are dropped
or reordered.  `x` is evaluated exactly once in both the original and the
rewritten form.

For signed integer `x`, `-(-x)` can technically overflow when `x ==
INT_MIN` (the negation of `INT_MIN` is undefined behavior in C99).  This
optimizer takes the same position as GCC/Clang's `-O1`: UB-free code is
assumed, so the fold is valid.

---

## Task — `src/optimize.c`: add `-(-x)` block in `optimize_expr`

Insert the following block **after** the "Double logical NOT" block (stage 147,
closing `}` after the `return neq;` path) and **before** the "Algebraic
identity folding" block (stage 145).

```c
    /* Double arithmetic negation: -(-x) -> x for non-constant x.
       When x is a literal, stage-144 folds both - applications in two passes;
       this rule fires only when the innermost child is not an INT_LITERAL. */
    if (node->type == AST_UNARY_OP &&
            node->child_count == 1 &&
            strcmp(node->value, "-") == 0) {
        ASTNode *inner = node->children[0];
        ASTNode *x;
        int fire;

        fire = (inner != NULL &&
                inner->type == AST_UNARY_OP &&
                inner->child_count == 1 &&
                strcmp(inner->value, "-") == 0 &&
                inner->children[0] != NULL &&
                inner->children[0]->type != AST_INT_LITERAL);

        if (fire) {
            x = inner->children[0];
            inner->children[0] = NULL; /* prevent double-free of x */
            ast_free(node);            /* frees outer - and inner - (not x) */
            return x;
        }
    }
```

No other changes to `src/optimize.c` are needed.

---

## Memory management

- `inner = node->children[0]` borrows the pointer; the slot is NOT nulled
  before `ast_free(node)`.
- `inner->children[0] = NULL` is set first to detach `x` from the inner `-`
  before the free traversal reaches it.
- `ast_free(node)` recurses: frees the outer `-` node → recurses into
  `node->children[0]` (inner `-`) → frees inner `-` node → recurses into
  `inner->children[0]` which is now `NULL` → stops.  `x` is NOT freed.
- `x` is returned directly; no new node is allocated.

This pattern is identical to the `!!x` block in Stage 147 (Task 1), with `"-"`
substituted for `"!"` and the return changed from building a `!=` node to
returning `x` directly.

---

## Result type convention

The returned node is `x` itself, which already carries the correct `decl_type`
and `is_unsigned` from parsing (or from a prior optimizer pass).  No type
fields need to be adjusted.

---

## Out of scope — do NOT do these in this stage

- `+x → x` (unary plus elimination) — a future stage.
- `-(+x) → -x`, `+(−x) → -x` — mixed unary chains — future.
- `~~x → x` (double bitwise NOT) — future.
- `!(!x)` where `x` is non-constant — note: `!!x` was already done in Stage 147
  as `(x != 0)`, which is not the same as structural `!!x`; the present rule
  does not interfere.
- Float literal negation folding — future.
- Overflow detection or UB warnings for `INT_MIN`.

---

## Bootstrap caveats

`src/optimize.c` must continue to compile cleanly under the C0 compiler
(previous stage binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations must appear at the top of a block before any executable
  statements.  The new block declares `inner`, `x`, and `fire` at the top of
  the outer `if` block before any executable statements.  The `if (fire)` body
  uses only assignments and a function call — no new declarations.
- `strcmp` is in `<string.h>`, already included.
- `ast_free` is declared in `"ast.h"`, already included.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
are unaffected.

### test_neg_fold_double_minus

```c
/* test/integration/test_neg_fold_double_minus/test_neg_fold_double_minus.c */
#include <stdio.h>
int main(void) {
    int x = 5;
    int y = -3;
    int a = -(-x);        /* non-zero positive var -> 5 */
    int b = -(-y);        /* negative var -> -3 */
    int c = -(-0);        /* const: stage-144 handles -> 0 */
    int d = -(-7);        /* const: stage-144 handles -> 7 */
    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `5 -3 0 7`

### test_neg_fold_triple_minus

Verify that three successive negations reduce correctly (bottom-up: inner pair
folds first, leaving a single `-`):

```c
/* test/integration/test_neg_fold_triple_minus/test_neg_fold_triple_minus.c */
#include <stdio.h>
int main(void) {
    int x = 4;
    int a = -(-(-x));     /* inner -(-x) -> x; then -(x) stays */
    printf("%d\n", a);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `-4`

### test_neg_fold_combined

Verify that double-minus folding composes with algebraic identity rules in a
single bottom-up pass:

```c
/* test/integration/test_neg_fold_combined/test_neg_fold_combined.c */
#include <stdio.h>
int main(void) {
    int x = 3;
    int a = -(-x) + 0;    /* -(-x) -> x; x + 0 -> x */
    int b = -(-x) * 1;    /* -(-x) -> x; x * 1 -> x */
    int c = -(-x) - x;    /* -(-x) -> x; x - x -> 0 */
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `3 3 0`

### Regression

Run the full test suite at both `-O0` and `-O1` via `./run_tests.sh`.  All
existing tests must continue to pass.

---

## Implementation order

1. In `src/optimize.c`, locate the closing `}` of the "Double logical NOT"
   block (stage 147), after the `return neq;` / `}` sequence, just before the
   "Algebraic identity folding" comment.
2. Insert the new `-(-x)` block between those two blocks.
3. Build: `cmake --build build`.
4. Smoke test:
   ```sh
   echo '#include <stdio.h>
   int main(void) {
       int x = 7; int y = -2;
       printf("%d %d %d\n", -(-x), -(-y), -(-(-x)));
       return 0; }' > /tmp/neg.c
   ./build/ccompiler -O1 /tmp/neg.c -o /tmp/neg.asm
   nasm -f elf64 /tmp/neg.asm -o /tmp/neg.o && gcc /tmp/neg.o -o /tmp/neg && /tmp/neg
   ```
   Expected output: `7 -2 -7`
5. Add integration tests.
6. Run `./run_tests.sh` — all tests pass.
7. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
8. Bump `VERSION_STAGE` to `"01480000"` in `src/version.c`.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Negation folding" item as
  complete (`[x]`), annotating Stage 148.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-148 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01480000"`).
