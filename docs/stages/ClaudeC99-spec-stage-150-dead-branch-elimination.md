# ClaudeC99 Stage 150 — Dead-Branch Elimination

## Goal

Implement dead-branch elimination in `optimize_stmt` for `if`, `while`, and
`for` statements whose controlling condition is a compile-time integer constant.
When the condition reduces to an `AST_INT_LITERAL` after child optimization,
the unreachable code path is freed and only the live path (or an empty block)
is returned.

This stage touches only `src/optimize.c`; no other source files change.  All
elimination is gated behind `-O1`; the `-O0` path (the default) is unaffected.

---

## Background

The stage-142 optimizer walks statements top-down in `optimize_stmt`.  For
control-flow statements the condition child is passed through `optimize_expr`
before the body children are optimized:

```
AST_IF_STATEMENT
    [0] condition  ← optimize_expr (may fold to AST_INT_LITERAL)
    [1] then-body  ← optimize_stmt
    [2] else-body  ← optimize_stmt  (optional)

AST_WHILE_STATEMENT
    [0] condition  ← optimize_expr (may fold to AST_INT_LITERAL)
    [1] body       ← optimize_stmt

AST_FOR_STATEMENT
    [0] init       ← optimize_stmt (NULL if omitted)
    [1] condition  ← optimize_expr (NULL if omitted; may fold to AST_INT_LITERAL)
    [2] update     ← optimize_expr (NULL if omitted)
    [3] body       ← optimize_stmt
```

After `optimize_expr` runs on the condition, any prior constant folding
(stages 143–149) may have reduced it to `AST_INT_LITERAL`.  This stage adds
post-optimization checks immediately after the existing child-optimization work
in each case, before the existing `return node`.

When a dead path is identified, the kept node (a body or empty block) is
returned and the dead nodes are freed via `ast_free`.

Related prior work:

- **Stage 142** — introduced the optimizer skeleton (`optimize_expr` /
  `optimize_stmt` / `optimize_translation_unit`).
- **Stage 143** — constant binary folding to `AST_INT_LITERAL`.
- **Stage 144** — constant unary folding.
- **Stage 147** — boolean/logical simplification (`x && 0`, `x || 1`, `!!x`).
- **Stage 148** — negation folding (`-(-x) → x`).
- **Stage 149** — ternary constant-condition folding (`const ? T : F`).

Stage 149 noted that constant-condition `if`/`while`/`for` elimination was
explicitly out of scope and deferred to a future stage.  This is that stage.

---

## Rule coverage

### Already handled by prior stages (do NOT re-implement)

| Pattern                      | Result         | Stage |
|------------------------------|----------------|-------|
| `const_a OP const_b`         | folded literal | 143   |
| `OP const`                   | folded literal | 144   |
| `x && 0`, `0 && x`          | `0`            | 147   |
| `x \|\| 1`, `1 \|\| x`      | `1`            | 147   |
| `-(-x)`                      | `x`            | 148   |
| `const ? T : F`              | selected branch | 149  |

### New rules added in this stage

#### `if` with constant condition

| Pattern                            | Result           | Notes |
|------------------------------------|------------------|-------|
| `if (nonzero) { S1 }`             | `S1`             | condition ≠ 0; no else |
| `if (nonzero) { S1 } else { S2 }` | `S1`             | condition ≠ 0; `S2` freed |
| `if (0) { S1 }`                   | `{}` (empty block) | `S1` freed |
| `if (0) { S1 } else { S2 }`      | `S2`             | `S1` freed |

#### `while` with constant zero condition

| Pattern         | Result           | Notes |
|-----------------|------------------|-------|
| `while (0) { S }` | `{}` (empty block) | body freed |

A constant non-zero `while` condition (infinite loop) is **not** eliminated —
removing an infinite loop changes observable behaviour.  Only `while (0)` is
safe to drop.

#### `for` with constant zero condition

| Pattern                      | Result   | Notes |
|------------------------------|----------|-------|
| `for (; 0; update) { S }`   | `{}` (empty block) | body, update, cond freed |
| `for (init; 0; update) { S }` | `init` | body, update, cond freed; init kept |

As with `while`, only the literal-zero condition is eliminated.

---

## Semantic notes

**Side effects in dead branches** — Dead branches eliminated here are
unreachable at runtime (the condition is a compile-time constant), so no
observable side effects are dropped.  This matches the assumption used by
GCC/Clang at `-O1`.

**Variable scope for `for` init** — When `for (int i = 0; 0; ...)` is reduced
to `int i = 0;`, the declaration is hoisted into the enclosing block scope.
In C99 the loop variable's scope normally ends at the closing brace of the
for statement, but because the loop body is also eliminated, no code outside
the (now dead) body can reference `i`.  The only observable change is that `i`
becomes visible in the rest of the enclosing scope — a benign extension at
`-O1` where the dead-code premise already applies.

**Empty block placeholder** — When a statement is eliminated entirely
(`while (0)`, `if (0)` with no else, `for (; 0; )`), `ast_new(AST_BLOCK, NULL)`
is returned in place of the dead statement.  The codegen for `AST_BLOCK` with
`child_count == 0` iterates zero times and emits no instructions.

---

## Task — `src/optimize.c`: add dead-branch checks in `optimize_stmt`

All three changes follow the same pattern: insert a constant-condition check
**after** the existing child optimization calls and **before** the `return node`
that ends each `case`.

### 1 — `AST_IF_STATEMENT` case (line ~417)

Replace the current case body:

```c
    case AST_IF_STATEMENT:
        /* children: [condition, then-body, (optional) else-body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        if (node->child_count > 2)
            node->children[2] = optimize_stmt(node->children[2]);
        return node;
```

With:

```c
    case AST_IF_STATEMENT: {
        /* children: [condition, then-body, (optional) else-body] */
        ASTNode *keep;
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        if (node->child_count > 2)
            node->children[2] = optimize_stmt(node->children[2]);
        if (node->children[0]->type == AST_INT_LITERAL) {
            long cval = strtol(node->children[0]->value, NULL, 0);
            if (cval != 0L) {
                /* Always true: keep then-branch, drop else-branch. */
                keep = node->children[1];
                node->children[1] = NULL;
                ast_free(node);
                return keep;
            } else {
                /* Always false: keep else-branch (or empty block). */
                keep = (node->child_count > 2)
                       ? node->children[2]
                       : ast_new(AST_BLOCK, NULL);
                if (node->child_count > 2) node->children[2] = NULL;
                ast_free(node);
                return keep;
            }
        }
        return node;
    }
```

### 2 — `AST_WHILE_STATEMENT` case (line ~425)

Replace the current case body:

```c
    case AST_WHILE_STATEMENT:
        /* children: [condition, body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        return node;
```

With:

```c
    case AST_WHILE_STATEMENT:
        /* children: [condition, body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        if (node->children[0]->type == AST_INT_LITERAL &&
                strtol(node->children[0]->value, NULL, 0) == 0L) {
            ast_free(node);
            return ast_new(AST_BLOCK, NULL);
        }
        return node;
```

### 3 — `AST_FOR_STATEMENT` case (line ~437)

Replace the current case body:

```c
    case AST_FOR_STATEMENT:
        /* children: [init|NULL, cond|NULL, update|NULL, body] */
        if (node->children[0]) node->children[0] = optimize_stmt(node->children[0]);
        if (node->children[1]) node->children[1] = optimize_expr(node->children[1]);
        if (node->children[2]) node->children[2] = optimize_expr(node->children[2]);
        node->children[3] = optimize_stmt(node->children[3]);
        return node;
```

With:

```c
    case AST_FOR_STATEMENT: {
        /* children: [init|NULL, cond|NULL, update|NULL, body] */
        ASTNode *init;
        if (node->children[0]) node->children[0] = optimize_stmt(node->children[0]);
        if (node->children[1]) node->children[1] = optimize_expr(node->children[1]);
        if (node->children[2]) node->children[2] = optimize_expr(node->children[2]);
        node->children[3] = optimize_stmt(node->children[3]);
        if (node->children[1] != NULL &&
                node->children[1]->type == AST_INT_LITERAL &&
                strtol(node->children[1]->value, NULL, 0) == 0L) {
            init = node->children[0];
            node->children[0] = NULL; /* detach init before ast_free */
            ast_free(node);           /* frees for-node, cond, update, body */
            return (init != NULL) ? init : ast_new(AST_BLOCK, NULL);
        }
        return node;
    }
```

---

## Memory management

The pattern in all three rules mirrors that of stage 149 and stage 145:

1. Save a pointer to the node(s) to keep.
2. Null out their slots in the parent node so `ast_free` does not recurse into
   them.
3. Call `ast_free(node)` to free the parent and all remaining (dead) children.
4. Return the kept node(s).

### `if` (nonzero condition)

- `node->children[1] = NULL` detaches then-branch.
- `ast_free(node)` frees: `if` node, condition literal, else-branch (if any).
- `keep` (then-branch) is returned.

### `if` (zero condition, with else)

- `node->children[2] = NULL` detaches else-branch.
- `ast_free(node)` frees: `if` node, condition literal, then-branch.
- `keep` (else-branch) is returned.

### `if` (zero condition, no else)

- `ast_free(node)` frees: `if` node, condition literal, then-branch.
- A fresh `ast_new(AST_BLOCK, NULL)` is returned; no aliasing issue.

### `while` (zero condition)

- `ast_free(node)` frees: `while` node, condition literal, body.
- A fresh `ast_new(AST_BLOCK, NULL)` is returned.

### `for` (zero condition)

- `init = node->children[0]`; `node->children[0] = NULL` detaches init.
- `ast_free(node)` frees: `for` node, condition literal, update, body.
- `init` (or a fresh empty block) is returned.

---

## Bootstrap caveats

`src/optimize.c` must compile cleanly under the C0 compiler (previous-stage
binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations must appear at the top of a block before any executable
  statements.  Each new block (`{ ... }`) introduced by `case` groups declares
  `keep` / `init` at the very top.
- `strtol` is in `<stdlib.h>`, already included.
- `ast_free`, `ast_new`, `AST_INT_LITERAL`, `AST_BLOCK`, `AST_IF_STATEMENT`,
  `AST_WHILE_STATEMENT`, `AST_FOR_STATEMENT` are declared in `"ast.h"`,
  already included.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
are unaffected.

### test_dead_if_true

Constant nonzero condition — then-branch runs; else-branch (with a value that
would corrupt the result if executed) is dropped:

```c
/* test/integration/test_dead_if_true/test_dead_if_true.c */
#include <stdio.h>
int main(void) {
    int a = 0;
    if (1) { a = 10; } else { a = 99; }
    int b = 0;
    if (-3) { b = 20; } else { b = 99; }
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `10 20`

### test_dead_if_false

Constant zero condition — else-branch runs; then-branch is dropped:

```c
/* test/integration/test_dead_if_false/test_dead_if_false.c */
#include <stdio.h>
int main(void) {
    int a = 0;
    if (0) { a = 99; } else { a = 10; }
    int b = 0;
    if (0) { b = 99; }
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `10 0`

### test_dead_while

Constant zero while condition — loop body is never entered:

```c
/* test/integration/test_dead_while/test_dead_while.c */
#include <stdio.h>
int main(void) {
    int n = 0;
    while (0) { n = 99; }
    printf("%d\n", n);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `0`

### test_dead_for

Constant zero for condition — init runs; body and update never execute:

```c
/* test/integration/test_dead_for/test_dead_for.c */
#include <stdio.h>
int main(void) {
    int x = 0;
    int n = 0;
    for (x = 5; 0; x++) { n = 99; }
    printf("%d %d\n", x, n);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `5 0`

### test_dead_for_no_init

For with zero condition and no init — entire loop disappears:

```c
/* test/integration/test_dead_for_no_init/test_dead_for_no_init.c */
#include <stdio.h>
int main(void) {
    int n = 0;
    for (; 0; ) { n = 99; }
    printf("%d\n", n);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `0`

### test_dead_combined

Dead-branch elimination composed with constant folding (stage 143):

```c
/* test/integration/test_dead_combined/test_dead_combined.c */
#include <stdio.h>
int main(void) {
    int a = 0;
    /* 3 - 3 -> 0 (stage 143); if(0) drops then-branch */
    if (3 - 3) { a = 99; } else { a = 10; }
    int b = 0;
    /* 2 * 1 -> 2 (stage 143); while(2) kept (non-zero, not eliminated) */
    int i = 0;
    /* 5 - 5 -> 0 (stage 143); while(0) drops loop */
    while (5 - 5) { b = 99; }
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `10 0`

### Regression

Run the full test suite via `./run_tests.sh`.  All existing tests must
continue to pass at both `-O0` and `-O1`.

---

## Implementation order

1. In `src/optimize.c`, apply the three case replacements described in the Task
   section above (if, while, for).
2. Build: `cmake --build build`.
3. Smoke test:
   ```sh
   printf '#include <stdio.h>\nint main(void){\n  int a=0;\n  if(0){a=1;}else{a=2;}\n  while(0){a=99;}\n  int x=0;\n  for(x=7;0;x++){a=99;}\n  printf("%%d %%d %%d\\n",a,x,a);\n  return 0;}\n' > /tmp/dead.c
   ./build/ccompiler -O1 /tmp/dead.c -o /tmp/dead.asm
   nasm -f elf64 /tmp/dead.asm -o /tmp/dead.o && gcc /tmp/dead.o -o /tmp/dead && /tmp/dead
   ```
   Expected output: `2 7 2`
4. Add the six integration tests.
5. Run `./run_tests.sh` — all tests pass.
6. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
7. Bump `VERSION_STAGE` to `"01500000"` in `src/version.c`.

---

## Out of scope — do NOT do these in this stage

- `do { S } while (0)` — this is a common idiom for multi-statement macros;
  the condition is checked *after* the body, so the body always runs at least
  once.  Eliminating it is not safe.
- Constant non-zero `while` / `for` — removing an infinite loop changes
  observable behaviour.  Only the zero case is safe.
- Non-literal conditions that are provably dead (e.g., a variable always 0
  due to prior assignment) — requires data-flow analysis, out of scope.
- Eliminating side-effect-free dead branches when the condition is
  non-constant — requires full dead-code analysis, out of scope.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Dead-branch elimination in
  `if`/`while`/`for` with constant condition" item as complete (`[x]`),
  annotating Stage 150; add a `## Stage 150` section after `## Stage 149`.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-150 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01500000"`).
