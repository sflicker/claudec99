# ClaudeC99 Stage 144 — Constant Unary Folding

## Goal

Complete the constant-unary-folding checklist item by folding `AST_UNARY_OP`
nodes whose single child is an `AST_INT_LITERAL` for the operators `-`, `+`,
and `!`.

`~` was already folded in stage 143 as part of the binary-folding rule block.
This stage unifies all four unary operators into a single rule block, replacing
the `~`-only block from stage 143 with a combined block that covers `-`, `+`,
`!`, and `~`.

This stage touches only `src/optimize.c`; no other source files change.  All
folding is gated behind `-O1`; the `-O0` path (the default) is unaffected.

---

## Background

The stage-142 optimizer walks the AST bottom-up in `optimize_expr`.  Stage 143
added binary folding and a `~`-only unary block.  By the time we inspect an
`AST_UNARY_OP`, its child has already been recursively simplified, so if it
emerged as an `AST_INT_LITERAL` the fold is unconditionally safe for `-`, `+`,
and `!`.

Integer literal values are stored as decimal strings in `node->value` and are
read back with `strtol(node->value, NULL, 0)`.  The `decl_type` field carries
the C type (`TYPE_INT`, `TYPE_LONG`, etc.) and `is_unsigned` marks unsigned
variants.

---

## Operator coverage

| Operator | `node->value` | Result | Notes |
|----------|--------------|--------|-------|
| `-`      | `"-"`        | `-val` | arithmetic negation; inherits operand type |
| `+`      | `"+"`        | `val`  | unary plus (no-op); inherits operand type |
| `!`      | `"!"`        | `0` or `1` | logical NOT; result is always `TYPE_INT` |
| `~`      | `"~"`        | `~val` | bitwise complement; already in stage 143, unified here |

---

## Task 1 — `src/optimize.c`: replace `~`-only block with unified unary block

In `optimize_expr`, remove the existing "Constant unary bitwise-NOT folding"
block (added in stage 143) and replace it with the following unified block.
The insertion point remains the same: after the logical short-circuit block and
before the final `return node;`.

```c
    /* Constant unary folding: -, +, !, ~ */
    if (node->type == AST_UNARY_OP &&
            node->child_count == 1 &&
            node->children[0]->type == AST_INT_LITERAL) {
        const char *op = node->value;
        long val = strtol(node->children[0]->value, NULL, 0);
        long result = 0;
        int is_logical = 0;
        int do_fold = 1;
        TypeKind operand_type     = node->children[0]->decl_type;
        int      operand_unsigned = node->children[0]->is_unsigned;
        ASTNode *lit;
        char buf[32];

        if      (strcmp(op, "-") == 0) { result = -val; }
        else if (strcmp(op, "+") == 0) { result = val; }
        else if (strcmp(op, "!") == 0) { result = (val == 0) ? 1 : 0; is_logical = 1; }
        else if (strcmp(op, "~") == 0) { result = ~val; }
        else                           { do_fold = 0; }

        if (do_fold) {
            snprintf(buf, sizeof(buf), "%ld", result);
            ast_free(node);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type   = is_logical ? TYPE_INT : operand_type;
            lit->is_unsigned = is_logical ? 0 : operand_unsigned;
            return lit;
        }
    }
```

No other changes to `src/optimize.c` are needed — the required `#include`
directives (`<stdio.h>`, `<string.h>`, `"util.h"`) were already added in
stage 143.

---

## Result type convention

- **`!`** (`is_logical == 1`): `decl_type = TYPE_INT`, `is_unsigned = 0`.
  Logical NOT always produces 0 or 1, which is an `int` in C.
- **`-`, `+`, `~`** (`is_logical == 0`): inherit `decl_type` and `is_unsigned`
  from the operand.  This matches the stage-143 convention for
  arithmetic/bitwise results.

---

## Memory note

`util_strdup(buf)` allocates a heap copy of the formatted string.  `ast_new()`
stores the pointer directly without copying, so the allocation must outlive the
node.  `ast_free(node)` frees the old binary node (and its children) before the
replacement literal is returned.

---

## Out of scope — do NOT do these in this stage

- Algebraic identities (`x + 0`, `x * 1`, etc.) — a later stage.
- Double-negation / double-NOT collapse (`--x`, `!!x`) — a later stage.
- Unary folding of `AST_FLOAT_LITERAL` operands — a later stage.
- `!` on pointer or floating-point operands.
- Overflow detection or UB warnings for folded arithmetic.
- Dead-branch elimination in `if`/`while`/`for` — a later stage.

---

## Bootstrap caveats

`src/optimize.c` must continue to compile cleanly under the C0 compiler
(previous stage binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations must appear at the top of a block before any statements.  All
  new declarations (`op`, `val`, `result`, `is_logical`, `do_fold`,
  `operand_type`, `operand_unsigned`, `lit`, `buf`) are declared at the top of
  the `if` block before any executable statements.
- `util_strdup` is available in C0 (added in stage 83).
- `snprintf` is in `<stdio.h>`, already included.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
are unaffected.

### test_fold_unary_minus

```c
/* test/integration/test_fold_unary_minus/test_fold_unary_minus.c */
#include <stdio.h>
int main(void) {
    int a = -3;
    int b = -0;
    int c = -(10);
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `-3 0 -10`

### test_fold_unary_plus

```c
/* test/integration/test_fold_unary_plus/test_fold_unary_plus.c */
#include <stdio.h>
int main(void) {
    int a = +5;
    int b = +0;
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `5 0`

### test_fold_unary_not

```c
/* test/integration/test_fold_unary_not/test_fold_unary_not.c */
#include <stdio.h>
int main(void) {
    int a = !0;
    int b = !1;
    int c = !42;
    int d = !(-1);
    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1 0 0 0`

### test_fold_unary_combined

Verify that unary folds compose correctly with binary folds in a single
bottom-up pass:

```c
/* test/integration/test_fold_unary_combined/test_fold_unary_combined.c */
#include <stdio.h>
int main(void) {
    int a = -(3 + 4);   /* binary fold → -7 */
    int b = !( 2 - 2);  /* binary fold → !(0) → 1 */
    int c = ~(1 << 3);  /* binary fold → ~8 → -9 */
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `-7 1 -9`

### Regression

Run the full test suite at both `-O0` and `-O1` via `./run_tests.sh`.  All
existing tests must continue to pass.

---

## Implementation order

1. In `src/optimize.c`, locate and remove the "Constant unary bitwise-NOT
   folding" block (the `~`-only block added in stage 143).
2. Insert the unified unary folding block in its place.
3. Build: `cmake --build build`.
4. Smoke test:
   ```sh
   echo '#include <stdio.h>
   int main(void) { printf("%d %d %d\n", -3, !0, +5); return 0; }' > /tmp/u.c
   ./build/ccompiler -O1 /tmp/u.c -o /tmp/u.asm
   nasm -f elf64 /tmp/u.asm -o /tmp/u.o && gcc /tmp/u.o -o /tmp/u && /tmp/u
   ```
   Expected output: `-3 1 5`
5. Add integration tests.
6. Run `./run_tests.sh` — all tests pass.
7. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
8. Bump `VERSION_STAGE` to `"01440000"` in `src/version.c`.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Constant unary folding" item
  (`- [ ] Constant unary folding — AST_UNARY_OP with constant operand: fold -, +, !, ~`)
  as complete (`[x]`), annotating Stage 144.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-144 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01440000"`).
