# ClaudeC99 Stage 149 — Conditional Expression Folding

## Goal

Implement constant-condition folding for the ternary operator in the
stage-142 optimizer: when the condition of an `AST_CONDITIONAL_EXPR` is a
compile-time integer constant, replace the entire `?:` node with the selected
branch and free the dead branch.

This stage touches only `src/optimize.c`; no other source files change.  All
folding is gated behind `-O1`; the `-O0` path (the default) is unaffected.

---

## Background

The stage-142 optimizer walks the AST bottom-up in `optimize_expr`.  Because
the walk is bottom-up, by the time `optimize_expr` visits an
`AST_CONDITIONAL_EXPR` node, all three children have already been optimized:

- `children[0]` — the condition — may have been folded to an `AST_INT_LITERAL`
  by stage-143 (constant binary folding), stage-144 (constant unary folding), or
  earlier rules.
- `children[1]` — the true branch — may have been simplified in any prior pass.
- `children[2]` — the false branch — same.

When the condition folds to a literal, both branches have been fully optimized
yet only one will ever be evaluated at runtime.  This rule eliminates the dead
branch at compile time.

Related prior work:

- **Stage 143** — folded constant binary and unary expressions to `AST_INT_LITERAL`.
- **Stage 144** — folded constant unary `-, +, !, ~` to `AST_INT_LITERAL`.
- **Stage 147** — simplified `x && 0 → 0`, `x || 1 → 1`, `!!x → (x != 0)`.
- **Stage 148** — folded `-(-x) → x` for non-constant `x`.

The `eval_const_init` path in `codegen.c` (line 5665) already handles
`AST_CONDITIONAL_EXPR` for constant initializers.  This stage adds the
analogous rule to the optimizer so that constant ternaries inside ordinary
expressions and statements are also eliminated.

---

## Rule coverage

### Already handled by prior stages (do NOT re-implement)

| Pattern                      | Result     | Stage |
|------------------------------|------------|-------|
| `const_a OP const_b`         | folded literal | 143 |
| `OP const`                   | folded literal | 144 |
| `x && 0`, `0 && x`          | `0`        | 147   |
| `x \|\| 1`, `1 \|\| x`      | `1`        | 147   |
| `-(-x)`                      | `x`        | 148   |

### New rule added in this stage

#### Constant condition ternary — `const ? T : F`

| Pattern            | Result | Notes |
|--------------------|--------|-------|
| `nonzero ? T : F`  | `T`    | condition is `AST_INT_LITERAL` with value ≠ 0; `F` is freed |
| `0 ? T : F`        | `F`    | condition is `AST_INT_LITERAL` with value == 0; `T` is freed |

The result is the surviving branch node itself — no new node is allocated.
The dead branch and the `?:` node (including the condition) are freed.

---

## Semantic note

In C99, the ternary operator evaluates its condition and exactly one of the two
branches.  When the condition is a compile-time constant, the dead branch is
never executed, so eliminating it does not change observable behaviour.  No
side effects are dropped: if the dead branch contained a function call or
assignment, the caller is assumed to have written dead-branch-free code (the
same UB-free assumption used by GCC/Clang at `-O1`).

The surviving branch already carries the correct `decl_type` and `is_unsigned`
from parsing (or from a prior optimizer pass).  No type fields need adjustment.

---

## Task — `src/optimize.c`: add constant-condition ternary block in `optimize_expr`

Insert the following block **after** the "Boolean/logical simplification" block
(stage 147, after the final closing `}` of the `x || 0 → != 0` branch) and
**before** the `return node;` that ends `optimize_expr`.

```c
    /* Conditional expression folding: const ? T : F -> selected branch.
       Bottom-up walk ensures the condition has already been folded; if it
       reduced to a literal we eliminate the dead branch entirely. */
    if (node->type == AST_CONDITIONAL_EXPR &&
            node->child_count == 3 &&
            node->children[0] != NULL &&
            node->children[0]->type == AST_INT_LITERAL) {
        long cval     = strtol(node->children[0]->value, NULL, 0);
        int  keep_idx = (cval != 0L) ? 1 : 2;
        ASTNode *keep = node->children[keep_idx];
        node->children[keep_idx] = NULL; /* detach before ast_free */
        ast_free(node);                  /* frees ?: node, condition, dead branch */
        return keep;
    }
```

No other changes to `src/optimize.c` are needed.

---

## Memory management

- `node->children[keep_idx] = NULL` detaches the surviving branch before the
  free traversal reaches it.
- `ast_free(node)` recurses:
  - frees the `?:` node itself
  - recurses into `children[0]` (the `AST_INT_LITERAL` condition) — freed
  - recurses into `children[keep_idx]` — now `NULL`, traversal stops
  - recurses into the other (dead) branch — freed in full
- `keep` is returned directly; no new node is allocated.

The `drop_idx` slot does not need to be explicitly computed or nulled: because
`keep_idx` is the only slot nulled, the dead branch is reachable from `node`
and freed by `ast_free` automatically.

This pattern follows the same idiom used in the algebraic identity rules in
Stage 145 (`node->children[0] = NULL; ast_free(node); return left;`).

---

## Result type convention

The returned `keep` node already has the correct `decl_type` and `is_unsigned`
set by the parser (or by a prior optimizer pass).  No type fields need to be
adjusted after the replacement.

---

## Out of scope — do NOT do these in this stage

- Constant-condition `if` statement elimination (`if (0) { ... }`) — a future
  stage; `AST_IF_STATEMENT` is a statement node handled in `optimize_stmt`, not
  `optimize_expr`.
- Folding when only one branch (not the condition) is constant — not applicable
  to this rule; both branches may be non-constant.
- Float / double condition literals — the condition of `?:` is always an
  integer after standard integral promotion; no FP condition literal can appear
  unless the user wrote `1.0 ? ... : ...`, which the parser does not produce as
  `AST_CONDITIONAL_EXPR` with an `AST_FLOAT_LITERAL` condition.
- Elimination of pure side-effect-free dead branches when the condition is
  non-constant — a full dead-code analysis, out of scope for this stage.

---

## Bootstrap caveats

`src/optimize.c` must continue to compile cleanly under the C0 compiler
(previous stage binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations must appear at the top of a block before any executable
  statements.  The new block declares `cval`, `keep_idx`, and `keep` at the
  top of the `if` body before any executable statements.
- `strtol` is in `<stdlib.h>`, already included.
- `ast_free` and `AST_INT_LITERAL` / `AST_CONDITIONAL_EXPR` are declared in
  `"ast.h"`, already included.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
are unaffected.

### test_cond_fold_true

Constant nonzero condition — true branch is selected; false branch (with side
effect that would corrupt `r` if executed) is dropped:

```c
/* test/integration/test_cond_fold_true/test_cond_fold_true.c */
#include <stdio.h>
int main(void) {
    int a = 1 ? 42 : 99;
    int b = 7 ? 10 : 20;
    int c = -1 ? 5 : 6;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `42 10 5`

### test_cond_fold_false

Constant zero condition — false branch is selected:

```c
/* test/integration/test_cond_fold_false/test_cond_fold_false.c */
#include <stdio.h>
int main(void) {
    int a = 0 ? 42 : 99;
    int b = 0 ? 10 : 20;
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `99 20`

### test_cond_fold_nested

Nested ternary with constant conditions — bottom-up walk folds innermost first:

```c
/* test/integration/test_cond_fold_nested/test_cond_fold_nested.c */
#include <stdio.h>
int main(void) {
    /* inner 0 ? 2 : 3 -> 3; then 1 ? 1 : 3 -> 1 */
    int a = 1 ? 1 : (0 ? 2 : 3);
    /* inner 1 ? 2 : 3 -> 2; then 0 ? 2 : 4 -> 4 */
    int b = 0 ? (1 ? 2 : 3) : 4;
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1 4`

### test_cond_fold_combined

Condition derived from prior constant folding — verify composition with
stage-143 binary folding:

```c
/* test/integration/test_cond_fold_combined/test_cond_fold_combined.c */
#include <stdio.h>
int main(void) {
    /* 2 + 3 -> 5 (stage 143); 5 ? 10 : 20 -> 10 (stage 149) */
    int a = (2 + 3) ? 10 : 20;
    /* 1 - 1 -> 0 (stage 143); 0 ? 10 : 20 -> 20 (stage 149) */
    int b = (1 - 1) ? 10 : 20;
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `10 20`

### Regression

Run the full test suite at both `-O0` and `-O1` via `./run_tests.sh`.  All
existing tests must continue to pass.

---

## Implementation order

1. In `src/optimize.c`, locate the closing `}` of the "Boolean/logical
   simplification" block (stage 147), after the `x || 0 → !=` path, just
   before the `return node;` that ends `optimize_expr`.
2. Insert the new conditional expression folding block between those two lines.
3. Build: `cmake --build build`.
4. Smoke test:
   ```sh
   echo '#include <stdio.h>
   int main(void) {
       int a = 1 ? 42 : 99;
       int b = 0 ? 42 : 99;
       int c = (3 - 3) ? 1 : 0;
       printf("%d %d %d\n", a, b, c);
       return 0; }' > /tmp/cond.c
   ./build/ccompiler -O1 /tmp/cond.c -o /tmp/cond.asm
   nasm -f elf64 /tmp/cond.asm -o /tmp/cond.o && gcc /tmp/cond.o -o /tmp/cond && /tmp/cond
   ```
   Expected output: `42 99 0`
5. Add integration tests.
6. Run `./run_tests.sh` — all tests pass.
7. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
8. Bump `VERSION_STAGE` to `"01490000"` in `src/version.c`.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Conditional expression folding"
  item as complete (`[x]`), annotating Stage 149.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-149 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01490000"`).
