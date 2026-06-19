# ClaudeC99 Stage 142 — Optimizer Infrastructure

## Goal

Introduce the **AST-level optimizer scaffolding**: a new `optimize.c` /
`include/optimize.h` module with a `optimize_translation_unit()` entry point
and recursive `optimize_expr()` / `optimize_stmt()` helpers that walk the AST
and return a (possibly replaced) node pointer.

In this stage the pass performs **no transformations** — it is a pure
tree-walking no-op that proves the infrastructure compiles, self-hosts, and
slots cleanly into the pipeline.  Subsequent stages (143+) will add the actual
constant-folding and dead-branch-elimination rules on top of this skeleton.

Also introduced: `-O0` / `-O1` command-line flags for `ccompiler` and `cc99`
that select the optimization level.  `-O1` enables the optimizer pass;
`-O0` (the default) skips it.  The flag is wired end-to-end so that later
optimization stages only need to add rules, not plumbing.

This is a **pure infrastructure stage** — no grammar changes, no new AST node
types, no changes to codegen, no new tests for observable code-quality
improvements (the pass is a no-op).  Tests verify that the pass runs without
errors and that the `-O0`/`-O1` flags are accepted.

---

## Background — current pipeline

```
Source → Preprocessor → Lexer → Parser → AST → CodeGen → NASM
```

`compile_one_file()` in `src/compiler.c` owns the pipeline for each source
file.  After `parse_translation_unit()` returns and before
`codegen_translation_unit()` is called, there is currently no transform pass.
This stage inserts the optimizer call at that point:

```
Source → Preprocessor → Lexer → Parser → AST → [Optimizer] → CodeGen → NASM
```

The optimizer owns no persistent state; it takes the root `ASTNode *` from the
parser, walks it, and returns (a possibly modified root) to `compile_one_file`.
In this stage "possibly modified" always means "unchanged".

---

## Task 1 — `include/optimize.h`

Create `include/optimize.h`:

```c
#ifndef CCOMPILER_OPTIMIZE_H
#define CCOMPILER_OPTIMIZE_H

#include "ast.h"

/*
 * optimize_translation_unit — AST-level optimization pass.
 *
 * Walks every function body in the translation unit and rewrites constant
 * expressions, dead branches, and algebraic identities.  The returned
 * pointer is the (possibly replaced) root; callers must use the return
 * value rather than the original pointer.
 *
 * opt_level: 0 = disabled (no-op, returns root unchanged).
 *            1 = AST-level constant folding and dead-code elimination.
 */
ASTNode *optimize_translation_unit(ASTNode *root, int opt_level);

#endif
```

The internal helpers `optimize_stmt` and `optimize_expr` are `static` in
`src/optimize.c` and are not exposed in the header.

---

## Task 2 — `src/optimize.c`

Create `src/optimize.c`.  In this stage the pass traverses the tree and
returns every node unchanged.  The traversal structure must be complete and
correct so that future stages can add rules by inserting early-return
replacements at the top of each case.

### Return-value replacement convention

Both helpers return `ASTNode *`.  The return value is the node to use in the
parent's child slot — it may be the same pointer that was passed in, or a
freshly allocated replacement node.  When a replacement is created, the old
node must be freed with `ast_free()`.  Callers always update their child
pointer with the return value:

```c
node->children[i] = optimize_expr(node->children[i]);
```

This pattern allows future stages to fold `AST_BINARY_OP` → `AST_INT_LITERAL`
without touching the traversal skeleton.

### `optimize_expr`

```c
static ASTNode *optimize_expr(ASTNode *node) {
    if (node == NULL) return NULL;

    /* Recurse into all children first (bottom-up). */
    for (int i = 0; i < node->child_count; i++)
        node->children[i] = optimize_expr(node->children[i]);

    /* Future stages insert transformation rules here, keyed on node->type.
     * Example structure (not yet active):
     *   if (node->type == AST_BINARY_OP) { ... return replacement; }
     */

    return node;
}
```

Expression node types to cover (all return the node unchanged in this stage):
`AST_INT_LITERAL`, `AST_FLOAT_LITERAL`, `AST_STRING_LITERAL`,
`AST_CHAR_LITERAL`, `AST_VAR_REF`, `AST_BINARY_OP`, `AST_UNARY_OP`,
`AST_ASSIGNMENT`, `AST_PREFIX_INC_DEC`, `AST_POSTFIX_INC_DEC`,
`AST_FUNCTION_CALL`, `AST_INDIRECT_CALL`, `AST_CAST`, `AST_ADDR_OF`,
`AST_DEREF`, `AST_ARRAY_INDEX`, `AST_SIZEOF_TYPE`, `AST_SIZEOF_EXPR`,
`AST_CONDITIONAL_EXPR`, `AST_COMMA_EXPR`, `AST_MEMBER_ACCESS`,
`AST_ARROW_ACCESS`, `AST_COMPOUND_LITERAL`, `AST_BUILTIN_VA_START`,
`AST_BUILTIN_VA_END`, `AST_BUILTIN_VA_COPY`, `AST_BUILTIN_VA_ARG`.

The generic child-recursion loop handles all of these uniformly; no per-type
dispatch is needed in this stage.

### `optimize_stmt`

```c
static ASTNode *optimize_stmt(ASTNode *node) {
    if (node == NULL) return NULL;

    switch (node->type) {

    case AST_BLOCK:
        for (int i = 0; i < node->child_count; i++)
            node->children[i] = optimize_stmt(node->children[i]);
        return node;

    case AST_IF_STATEMENT:
        /* children: [condition, then-body, (optional) else-body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        if (node->child_count > 2)
            node->children[2] = optimize_stmt(node->children[2]);
        return node;

    case AST_WHILE_STATEMENT:
        /* children: [condition, body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        return node;

    case AST_DO_WHILE_STATEMENT:
        /* children: [body, condition] */
        node->children[0] = optimize_stmt(node->children[0]);
        node->children[1] = optimize_expr(node->children[1]);
        return node;

    case AST_FOR_STATEMENT:
        /* children: [init|NULL, cond|NULL, update|NULL, body] */
        if (node->children[0]) node->children[0] = optimize_stmt(node->children[0]);
        if (node->children[1]) node->children[1] = optimize_expr(node->children[1]);
        if (node->children[2]) node->children[2] = optimize_expr(node->children[2]);
        node->children[3] = optimize_stmt(node->children[3]);
        return node;

    case AST_SWITCH_STATEMENT:
        /* children: [discriminant, body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        return node;

    case AST_RETURN_STATEMENT:
        if (node->child_count > 0)
            node->children[0] = optimize_expr(node->children[0]);
        return node;

    case AST_EXPRESSION_STMT:
        if (node->child_count > 0)
            node->children[0] = optimize_expr(node->children[0]);
        return node;

    case AST_DECLARATION:
    case AST_DECL_LIST:
        /* Initializer expressions are children of DECLARATION nodes. */
        for (int i = 0; i < node->child_count; i++)
            node->children[i] = optimize_expr(node->children[i]);
        return node;

    case AST_LABEL_STATEMENT:
    case AST_CASE_SECTION:
    case AST_DEFAULT_SECTION:
        /* children: [body-statement, ...] */
        for (int i = 0; i < node->child_count; i++)
            node->children[i] = optimize_stmt(node->children[i]);
        return node;

    case AST_BREAK_STATEMENT:
    case AST_CONTINUE_STATEMENT:
    case AST_GOTO_STATEMENT:
        return node;   /* no children to optimize */

    default:
        /* Any node type not listed above (e.g. a nested expression used
         * directly as a statement) — recurse generically. */
        for (int i = 0; i < node->child_count; i++)
            node->children[i] = optimize_expr(node->children[i]);
        return node;
    }
}
```

### `optimize_translation_unit`

```c
ASTNode *optimize_translation_unit(ASTNode *root, int opt_level) {
    if (opt_level == 0) return root;

    /* root is AST_TRANSLATION_UNIT; children are top-level declarations
     * and function definitions. */
    for (int i = 0; i < root->child_count; i++) {
        ASTNode *decl = root->children[i];
        if (decl->type == AST_FUNCTION_DECL) {
            /* Last child of AST_FUNCTION_DECL is the body block (if this
             * is a definition, not a prototype).  Prototypes have no body;
             * detect by checking whether the last child is AST_BLOCK. */
            int last = decl->child_count - 1;
            if (last >= 0 && decl->children[last]->type == AST_BLOCK)
                decl->children[last] = optimize_stmt(decl->children[last]);
        }
    }
    return root;
}
```

---

## Task 3 — `src/compiler.c`: add `-O0` / `-O1` flag

### Changes to `main()`

1. Add a local variable `int opt_level = 0;` alongside `print_ast`, `print_tokens`, etc.

2. Add two new flag arms to the argument-parsing loop (before the positional-argument fallthrough):

```c
} else if (strcmp(argv[i], "-O0") == 0) {
    opt_level = 0;
} else if (strcmp(argv[i], "-O1") == 0) {
    opt_level = 1;
```

3. Pass `opt_level` to `compile_one_file()`.

4. Update the usage string at line 433 to include `[-O0|-O1]`.

### Changes to `compile_one_file()`

Add `int opt_level` as a new parameter.

After the parse-error guard and before the `print_ast` branch, call the
optimizer:

```c
/* Optimize (no-op when opt_level == 0). */
#include "optimize.h"   /* add to top of file */
...
ast = optimize_translation_unit(ast, opt_level);
```

The full updated signature:

```c
static int compile_one_file(const char *source_file,
                            int print_ast, int print_tokens,
                            int warnings_are_errors,
                            int opt_level,
                            const char **defines, int n_defines,
                            const char **include_dirs, int n_include_dirs);
```

Insertion point within `compile_one_file` (after parse-error check, before
`print_ast` branch, around line 299):

```c
    if (g_error_count > 0) { ... return 1; }

    /* Optimize AST (no-op at -O0). */
    ast = optimize_translation_unit(ast, opt_level);

    if (print_ast) {
        ast_pretty_print(ast, 0);
        ...
    }
```

Update the call site in `main()` to pass `opt_level`.

### Update `CMakeLists.txt`

Add `src/optimize.c` to the source list for the `ccompiler` target.

---

## Task 4 — `bin/cc99`: pass `-O0` / `-O1` through

In the `cc99` bash wrapper's argument-parsing `case` block, add:

```bash
-O0|-O1)
    compiler_flags+=("$1"); shift ;;
```

Add `-O0|-O1` to the usage string in the `usage()` function.

The optimizer flag is a `ccompiler` flag (not a linker flag), so it is
accumulated in `compiler_flags` and forwarded to every `"$COMPILER"` invocation.

---

## Out of scope — do NOT do these in this stage

- Any actual constant folding, dead-branch elimination, or algebraic-identity
  rules — those are stage 143+.
- `-O2` or higher levels — reserved for the Level-3 IR optimizer.
- Optimization of file-scope initializers (handled by `codegen_emit_data`, not
  by the AST optimizer).
- A `-O` flag without a digit (GCC accepts bare `-O` as `-O1`) — omit for
  simplicity; only `-O0` and `-O1` are recognized.
- Warning or diagnostic output from the optimizer — no new messages in this
  stage.
- `--print-ast` interaction with the optimizer: the AST is printed *after*
  optimization (`optimize_translation_unit` runs before the `print_ast` branch)
  so the printed tree will reflect any future optimizations.  This is the
  correct behavior and requires no special handling.

---

## Bootstrap caveats

The compiler must continue to self-host (C0→C1→C2).  `src/optimize.c` must
compile cleanly under the C0 compiler (the previous stage binary).

- No VLAs, no C99 compound literals, no `//` comments in new `.c` files.
- The `switch` in `optimize_stmt` uses only constant integer cases — fine.
- The generic child-loop `for (int i = 0; ...)` is valid C99 and handled by
  the current compiler.
- `include/optimize.h` must guard with `#ifndef CCOMPILER_OPTIMIZE_H`.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Flag acceptance tests (valid)

Verify that `ccompiler` accepts both flags without error:

```sh
# -O0: explicit no-optimization (default behavior)
echo 'int main(void) { return 42; }' > /tmp/t.c
./build/ccompiler -O0 /tmp/t.c
# expect: compiled: /tmp/t.c -> /tmp/t.asm; exit 0

# -O1: optimization enabled (no-op in this stage, same output)
./build/ccompiler -O1 /tmp/t.c
# expect: compiled: /tmp/t.c -> /tmp/t.asm; exit 0
```

### Output equivalence test

When run on the same source, `-O0` and `-O1` must produce identical NASM
output in this stage (since the optimizer is a no-op):

```sh
./build/ccompiler -O0 /tmp/t.c -o /tmp/t_O0.asm
./build/ccompiler -O1 /tmp/t.c -o /tmp/t_O1.asm
diff /tmp/t_O0.asm /tmp/t_O1.asm
# expect: no diff
```

### cc99 flag passthrough test

```sh
bin/cc99 -O1 -o /tmp/hello /tmp/t.c
/tmp/hello; echo $?   # expect 42
```

### Regression: existing test suite

Run the full test suite (`./run_tests.sh`) at both `-O0` and `-O1`.  All
existing tests must pass at both levels (since no transformations are applied).

The test runner does not need to be updated for this stage — optimization
flags are not yet exercised by integration tests.  A follow-up after stage 143
(first real optimization rule) will add `-O1` variants of select tests.

---

## Implementation order

1. `include/optimize.h` — create the header.
2. `src/optimize.c` — create the module with the no-op traversal.
3. `CMakeLists.txt` — add `src/optimize.c` to the build.
4. `src/compiler.c` — add `#include "optimize.h"`, add `opt_level` variable,
   parse `-O0`/`-O1`, thread `opt_level` through `compile_one_file`, call
   `optimize_translation_unit`.
5. `bin/cc99` — add `-O0`/`-O1` passthrough.
6. Build and verify: `cmake --build build && ./build/ccompiler -O1 <any-test-file>`.
7. Run `./build.sh --mode=self-host` — C0→C1→C2 must still pass.
8. Run `./run_tests.sh` — all existing tests must pass.
9. `src/version.c` — bump `VERSION_STAGE` to `"01420000"`.

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — note `-O0`/`-O1` flags in the usage summary.
- **`docs/outlines/checklist.md`** — mark the infrastructure item under
  "Optimize Level 1" as complete.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-142 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01420000"`).
