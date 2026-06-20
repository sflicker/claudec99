# ClaudeC99 Stage 154 — Unreachable Statement Removal

## Goal

In `optimize_stmt`, when iterating through the children of an `AST_BLOCK`,
detect any direct-child terminal statement (`return`, `break`, `continue`,
`goto`) and drop all subsequent siblings up to the next `AST_LABEL_STATEMENT`
(exclusive).  The label itself — and everything after it — is kept because it
is reachable via a `goto` from elsewhere.  Dead siblings are freed via
`ast_free`.

This stage touches only `src/optimize.c`; no other source files change.  All
removal is gated behind `-O1`; the `-O0` default path is unaffected.

---

## Background

The stage-142 optimizer's `AST_BLOCK` case iterates through block children
and passes each through `optimize_stmt`:

```c
case AST_BLOCK: {
    int saved_count = g_const_count;
    for (i = 0; i < node->child_count; i++)
        node->children[i] = optimize_stmt(node->children[i]);
    g_const_count = saved_count;
    return node;
}
```

After stages 143–153, constant folding and dead-branch elimination may have
simplified code considerably, but any statements textually after `return`,
`break`, `continue`, or `goto` in the same block are still carried through the
optimizer and emitted by codegen, producing dead instructions.  This stage adds
a simple forward scan after each terminal to strip those dead siblings.

**Why "up to the next label"?**  A `goto` elsewhere in the function can jump
to a named label inside a block, making the label and everything after it
reachable even if control never falls through from the preceding statement.
Only the code *between* the terminal and the next label is provably dead.

Related prior work:

- **Stage 142** — optimizer skeleton: `optimize_stmt` / `optimize_expr` /
  `optimize_translation_unit`.
- **Stage 150** — dead-branch elimination: `if/while/for` with constant
  condition.

---

## Termination rules

A statement is **terminal** if it is one of:

| AST node type          | C statement |
|------------------------|-------------|
| `AST_RETURN_STATEMENT` | `return …;` |
| `AST_BREAK_STATEMENT`  | `break;`    |
| `AST_CONTINUE_STATEMENT` | `continue;` |
| `AST_GOTO_STATEMENT`   | `goto lbl;` |

Only **direct** children of an `AST_BLOCK` are inspected.  A nested block
that internally ends in `return` (e.g., an `if`-body simplified by stage 150
into `{ return 0; }`) is *not* treated as terminal at the outer block level;
that generalization requires flow-analysis and is out of scope.

---

## Already handled by prior stages (do NOT re-implement)

| Pattern                      | Result           | Stage |
|------------------------------|------------------|-------|
| `const_a OP const_b`         | folded literal   | 143   |
| `OP const`                   | folded literal   | 144   |
| `x && 0`, `0 && x`          | `0`              | 147   |
| `x \|\| 1`, `1 \|\| x`      | `1`              | 147   |
| `-(-x)`                      | `x`              | 148   |
| `const ? T : F`              | selected branch  | 149   |
| `if/while/for (0)`           | dead-branch drop | 150   |
| `sizeof(type/expr)`          | folded literal   | 151   |
| `const`-scalar-local `VAR_REF` | propagated literal | 152 |
| `(integer_type) integer_literal` | folded literal | 153 |

---

## Semantic notes

**Observable behaviour** — Statements between a terminal and the next label
are unreachable at runtime by definition; dropping them cannot change any
observable side effect.

**Label stop rule** — The scan stops when it encounters an `AST_LABEL_STATEMENT`.
Labels are always reachable via `goto`, so they must not be removed.
Everything after the label continues to be processed normally by the loop's
subsequent iterations.

**Multiple dead zones** — A block may contain more than one dead zone (e.g.,
`return; dead1; label_a: return; dead2; label_b: ...`).  The outer `for` loop
handles this naturally: after compacting the first dead zone the loop advances
to `label_a`, continues optimizing, encounters the second terminal, and strips
`dead2` in a second pass through the inner logic.

**`do { ... } while (0)` body** — `continue` inside a `do…while` block is
terminal; if such a `continue` appears at block level, subsequent siblings are
dropped.  This is correct: after `continue` the condition is evaluated, not
the remaining body statements.

**`AST_BLOCK` child_count update** — After compaction, `node->child_count` is
decremented to reflect the removed children.  The `children` array itself is
not reallocated; the compacted tail is left valid.  Codegen and all consumers
already use `child_count` as the authoritative length.

---

## Task — `src/optimize.c`

### 1 — Static helper: `is_terminal_stmt`

Insert the following helper **before** `optimize_stmt` (e.g., immediately
before its definition, after the `const_prop_lookup` and `cast_value_safe`
helpers from prior stages):

```c
/* Return 1 if node is an unconditional transfer-of-control statement at the
   current block level.  Used to identify the start of a dead-code zone. */
static int is_terminal_stmt(ASTNode *node) {
    if (node == NULL) return 0;
    return (node->type == AST_RETURN_STATEMENT   ||
            node->type == AST_BREAK_STATEMENT    ||
            node->type == AST_CONTINUE_STATEMENT ||
            node->type == AST_GOTO_STATEMENT);
}
```

### 2 — `AST_BLOCK` case in `optimize_stmt`

Replace the current `AST_BLOCK` case:

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

With:

```c
    case AST_BLOCK: {
        /* Save g_const_count so entries added in this block are invisible
           in the enclosing scope after the block exits. */
        int saved_count = g_const_count;
        int j, k;
        for (i = 0; i < node->child_count; i++) {
            node->children[i] = optimize_stmt(node->children[i]);
            if (is_terminal_stmt(node->children[i])) {
                /* Free dead siblings [i+1 .. k-1] where k is the first label
                   or the end of the block, then compact the children array. */
                k = i + 1;
                while (k < node->child_count &&
                       node->children[k]->type != AST_LABEL_STATEMENT) {
                    ast_free(node->children[k]);
                    k++;
                }
                j = i + 1;
                while (k < node->child_count)
                    node->children[j++] = node->children[k++];
                node->child_count = j;
            }
        }
        g_const_count = saved_count;
        return node;
    }
```

### 3 — Update file-top comment block

Add to the existing comment at the top of `optimize.c`:

```
 * Stage 154: unreachable statement removal -- direct-child terminal nodes
 *            (return/break/continue/goto) in AST_BLOCK trigger a forward
 *            scan that frees all subsequent siblings up to the next label.
```

---

## Memory management

The inner `while` loop frees each dead child directly via `ast_free(node->children[k])`.
`ast_free` recursively frees all descendants, so no leak occurs in nested
dead subtrees.

The compaction `while` loop overwrites the freed slots with the surviving
suffix `[k .. child_count-1]`.  After compaction `node->child_count = j`
(where `j = (i+1) + number_of_surviving_suffix_elements`).  The now-unused
trailing slots in `node->children` still hold their old (freed) pointer values,
but they are never accessed because `child_count` is the authoritative bound.
No double-free occurs: the freed nodes' pointers are not referenced again.

---

## Bootstrap caveats

`src/optimize.c` must compile cleanly under the C0 compiler (previous-stage
binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations (`j`, `k`) must appear at the top of the `case AST_BLOCK: {`
  block, before the `for` loop — C89 requires all declarations before any
  executable statements within a block.
- `ast_free` and `AST_LABEL_STATEMENT`, `AST_RETURN_STATEMENT`,
  `AST_BREAK_STATEMENT`, `AST_CONTINUE_STATEMENT`, `AST_GOTO_STATEMENT` are
  declared in `"ast.h"`, already included.
- `is_terminal_stmt` is a `static` helper in the same file; no header change.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
(including all `-O0` tests) must be unaffected.

### test_unreachable_return

Multiple dead statements after `return` at block level — function returns the
first value; dead assignments are never executed:

```c
/* test/integration/test_unreachable_return/test_unreachable_return.c */
#include <stdio.h>
int foo(void) {
    return 1;
    return 2; /* dead */
    return 3; /* dead */
}
int main(void) {
    printf("%d\n", foo());
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1`

### test_unreachable_break

Dead statement after `break` inside a loop body — the loop exits after the
first iteration; `n = 99` must not execute:

```c
/* test/integration/test_unreachable_break/test_unreachable_break.c */
#include <stdio.h>
int main(void) {
    int n = 0;
    while (1) {
        n = 5;
        break;
        n = 99; /* dead */
    }
    printf("%d\n", n);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `5`

### test_unreachable_continue

Dead statement after `continue` inside a `for` loop — the increment and
condition check follow `continue`; `n = 99` must not execute:

```c
/* test/integration/test_unreachable_continue/test_unreachable_continue.c */
#include <stdio.h>
int main(void) {
    int n = 0;
    int i;
    for (i = 0; i < 3; i++) {
        n++;
        continue;
        n = 99; /* dead */
    }
    printf("%d\n", n);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `3`

### test_unreachable_goto

Multiple dead statements between a `goto` and its target label — only the
code before the jump and after the label executes:

```c
/* test/integration/test_unreachable_goto/test_unreachable_goto.c */
#include <stdio.h>
int main(void) {
    int a = 1;
    goto skip;
    a = 99; /* dead */
    a = 88; /* dead */
    skip:
    printf("%d\n", a);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1`

### test_unreachable_label_stop

Verify that the first label encountered stops the dead-code sweep — code after
the label is still executed, proving the label and its successors are kept:

```c
/* test/integration/test_unreachable_label_stop/test_unreachable_label_stop.c */
#include <stdio.h>
int main(void) {
    int a = 0;
    int b = 0;
    goto first;
    a = 99;    /* dead -- before first label */
    first:
    a = 1;     /* alive -- reached via goto */
    goto second;
    b = 99;    /* dead -- between labels */
    second:
    b = 2;     /* alive -- reached via goto */
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1 2`

### Regression

Run the full test suite via `./run_tests.sh`.  All existing tests must
continue to pass at both `-O0` and `-O1`.  In particular:

- Code at `-O0` must be unaffected: the check is inside the `opt_level != 0`
  path that enters `optimize_stmt` at all.
- Labels must never be dropped: even if all preceding statements in a block
  terminate, a label is always preserved.
- Only direct children of `AST_BLOCK` are inspected: a nested `if`-body or
  sub-block that happens to end in `return` does not trigger removal of outer
  siblings.

---

## Implementation order

1. In `src/optimize.c`, add the `is_terminal_stmt` static helper before
   `optimize_stmt`.
2. Replace the `AST_BLOCK` case body with the new version (add `int j, k;`
   declarations and the compaction logic).
3. Update the file-top comment block.
4. Build: `cmake --build build`.
5. Smoke test:
   ```sh
   printf '#include <stdio.h>\nint f(void){return 1;return 99;}\nint main(void){printf("%%d\\n",f());return 0;}\n' > /tmp/dead.c
   ./build/ccompiler -O1 /tmp/dead.c -o /tmp/dead.asm
   nasm -f elf64 /tmp/dead.asm -o /tmp/dead.o && gcc /tmp/dead.o -o /tmp/dead && /tmp/dead
   ```
   Expected output: `1`
6. Add the five integration tests.
7. Run `./run_tests.sh` — all tests pass.
8. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
9. Bump `VERSION_STAGE` to `"01540000"` in `src/version.c`.

---

## Out of scope — do NOT do these in this stage

- **Block-ending-in-terminal at outer level** — if a child of the block is
  itself an `AST_BLOCK` (e.g., reduced from `if (1) { return 0; }` by stage
  150), its internal terminal does not make it terminal at the parent level.
  Detecting this requires recursive flow analysis.
- **Function-level unreachable code after the last `return`** — already
  covered in practice by this stage, since the last `return` in a function body
  causes subsequent siblings to be dropped; no separate treatment needed.
- **Unreachable instructions in codegen output** — stripping `jmp`/`ret`
  followed by non-label instructions in the emitted assembly is a separate
  stage (`- [ ] Unreachable instruction removal` in the checklist) and is
  explicitly out of scope here.
- **Unreachable code warning** — the checklist notes `- [ ] Unreachable code
  warning`; emitting diagnostics is out of scope.
- **Dead code across labels** — code after a label that is itself never the
  target of any `goto` is theoretically dead, but proving it requires tracking
  all `goto` targets in the function; out of scope.
- **`switch` fallthrough** — `break` inside a `switch` case body is handled by
  the same logic (it is `AST_BREAK_STATEMENT` at block level), but detecting
  fallthrough from one case to the next is not implemented here.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Unreachable statement removal
  after `return`, `break`, `continue`, `goto`" item as complete (`[x]`),
  annotating Stage 154; add a `## Stage 154` section after `## Stage 153`.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-154 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01540000"`).
