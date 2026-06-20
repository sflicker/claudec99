# ClaudeC99 Stage 151 — sizeof Constant Folding

## Goal

Fold `AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` nodes to `AST_INT_LITERAL` in
`optimize_expr` wherever the size is determinable without runtime symbol-table
access.  After this stage, constant expressions that include a `sizeof(type)` or
`sizeof(literal)` sub-expression are fully visible to the constant-folding,
strength-reduction, and dead-branch-elimination passes (stages 143–150).

This stage touches only `src/optimize.c`; no other source files change.  All
folding is gated behind `-O1`; the `-O0` default path is unaffected.

---

## Background

In C99 (without VLAs, which this compiler does not support) the result of every
`sizeof` expression is a compile-time constant.  The codegen currently handles
both sizeof node types at code-generation time by emitting `mov rax, N` directly.
Because the optimizer runs before codegen, this means the constant value is
invisible to optimizer passes:

```c
if (sizeof(long) == 8) { ... }  /* condition is not an AST_INT_LITERAL; stage 150 cannot eliminate it */
int buf[sizeof(int) * 64];      /* size expression is not a literal; stage 143 cannot fold it */
```

After this stage, `optimize_expr` replaces sizeof nodes with `AST_INT_LITERAL`
nodes where the size can be determined from information already present on the
AST node.  The codegen `AST_SIZEOF_TYPE` / `AST_SIZEOF_EXPR` handlers remain
unchanged as a fallback for any nodes the optimizer could not fold (currently:
`AST_SIZEOF_EXPR` whose operand is a non-literal expression such as a variable
reference).

The replacement literal carries `decl_type = TYPE_LONG` and `is_unsigned = 1`
to match the semantics set by codegen (`sizeof` yields `unsigned size_t`, C99
§6.5.3.4).

Related prior work:

- **Stage 131** — sizeof marked `is_unsigned = 1` in codegen; unsigned
  arithmetic rules applied to sizeof results.
- **Stage 142** — optimizer skeleton: `optimize_expr` / `optimize_stmt` /
  `optimize_translation_unit`.
- **Stage 143** — constant integer binary folding; depends on having literal
  operands to fold.
- **Stage 150** — dead-branch elimination for `if`/`while`/`for` with
  constant-literal condition; depends on the condition being `AST_INT_LITERAL`.

---

## Rule coverage

### `AST_SIZEOF_TYPE` — complete folding (all cases)

The parser stores the resolved type on the sizeof node itself
(`node->decl_type`, and `node->full_type` for aggregate / array types).  All
cases are foldable at optimize time without any symbol-table access.

| Type specifier               | `node->decl_type`        | Folded size |
|------------------------------|--------------------------|-------------|
| `sizeof(_Bool)`              | `TYPE_BOOL`              | 1           |
| `sizeof(char)`               | `TYPE_CHAR`              | 1           |
| `sizeof(short)`              | `TYPE_SHORT`             | 2           |
| `sizeof(int)`                | `TYPE_INT`               | 4           |
| `sizeof(long)`               | `TYPE_LONG`              | 8           |
| `sizeof(long long)`          | `TYPE_LONG_LONG`         | 8           |
| `sizeof(unsigned long long)` | `TYPE_UNSIGNED_LONG_LONG`| 8           |
| `sizeof(float)`              | `TYPE_FLOAT`             | 4           |
| `sizeof(double)`             | `TYPE_DOUBLE`            | 8           |
| `sizeof(void *)`             | `TYPE_POINTER`           | 8           |
| `sizeof(struct S)`           | `TYPE_STRUCT`            | `full_type->size` |
| `sizeof(union U)`            | `TYPE_UNION`             | `full_type->size` |
| `sizeof(int[10])`            | `TYPE_ARRAY`             | `full_type->size` |

The size switch in the optimizer matches the switch used by `codegen_expression`
for `AST_SIZEOF_TYPE` (lines ~3440–3450 of `codegen.c`), with one deliberate
correction: `TYPE_DOUBLE` is handled explicitly as 8 bytes.  The codegen switch
omits it and falls through to `default: sz = 4`, which is incorrect; the
optimizer helper fixes this for `-O1`.  See the "Out of scope" section for the
decision not to fix the codegen path in this stage.

### `AST_SIZEOF_EXPR` — partial folding (literal operands)

`AST_SIZEOF_EXPR` requires knowing the type of an arbitrary expression.  The
optimizer has no symbol-table access, so it can fold only the subset of cases
where the child expression already carries reliable type information:

| Child node type             | Size derivation                        |
|-----------------------------|----------------------------------------|
| `AST_STRING_LITERAL`        | `child->byte_length + 1`              |
| `AST_INT_LITERAL`           | Scalar size from `child->decl_type`   |
| `AST_CHAR_LITERAL`          | 4 (char literal promotes to `int` in `sizeof`) |

For all other `AST_SIZEOF_EXPR` children (variable references, binary/unary
expressions, function calls, etc.) the optimizer returns the node unchanged.
The existing codegen handlers for `AST_SIZEOF_EXPR` continue to process these
cases at code-generation time, preserving existing behavior.

### Already handled by prior stages (do NOT re-implement)

| Pattern                      | Result         | Stage |
|------------------------------|----------------|-------|
| `const_a OP const_b`         | folded literal | 143   |
| `OP const`                   | folded literal | 144   |
| `x && 0`, `0 && x`           | `0`            | 147   |
| `x \|\| 1`, `1 \|\| x`       | `1`            | 147   |
| `-(-x)`                      | `x`            | 148   |
| `const ? T : F`              | selected branch| 149   |
| `if/while/for (0)`           | dead-branch drop| 150  |

---

## Semantic notes

**Result type** — `sizeof` always yields `unsigned size_t`.  The replacement
`AST_INT_LITERAL` node must have `decl_type = TYPE_LONG` and `is_unsigned = 1`
to preserve the unsigned arithmetic semantics applied to sizeof results since
stage 131.

**Struct/union/array completeness** — `AST_SIZEOF_TYPE` for a struct/union with
`full_type->size == 0` is already rejected at parse time with an "incomplete
type" error.  The optimizer therefore assumes `full_type->size > 0` when
`full_type != NULL`.

**`AST_SIZEOF_EXPR` with `AST_INT_LITERAL` child** — after the bottom-up walk,
a constant expression operand may have been folded to `AST_INT_LITERAL`.  This
child retains its `decl_type` (set by the parser from the integer suffix: no
suffix → `TYPE_INT`, `L` suffix → `TYPE_LONG`, etc.).  The optimizer uses this
to determine the sizeof value.

**`AST_CHAR_LITERAL`** — the C99 type of a character constant is `int`, so
`sizeof('a') == sizeof(int) == 4`.  The optimizer always returns 4 for a
`AST_CHAR_LITERAL` operand.

---

## Task — `src/optimize.c`: add sizeof folding in `optimize_expr`

### 1 — Static helper: `sizeof_scalar_size`

Insert a new static helper **before** `optimize_expr`:

```c
/* Map a scalar TypeKind to its sizeof value.
   Replicates codegen_expression's AST_SIZEOF_TYPE scalar switch. */
static int sizeof_scalar_size(TypeKind k) {
    switch (k) {
    case TYPE_BOOL:               return 1;
    case TYPE_CHAR:               return 1;
    case TYPE_SHORT:              return 2;
    case TYPE_LONG:               return 8;
    case TYPE_LONG_LONG:          return 8;
    case TYPE_UNSIGNED_LONG_LONG: return 8;
    case TYPE_POINTER:            return 8;
    case TYPE_DOUBLE:             return 8;
    default:                      return 4; /* TYPE_INT, TYPE_FLOAT */
    }
}
```

### 2 — Static helper: `make_sizeof_literal`

Also before `optimize_expr`:

```c
/* Create an AST_INT_LITERAL whose value and type match what sizeof returns.
   The caller must pass in the computed byte count sz > 0.
   Returns a freshly allocated node. */
static ASTNode *make_sizeof_literal(int sz) {
    char buf[16];
    ASTNode *lit;
    snprintf(buf, sizeof(buf), "%d", sz);
    lit = ast_new(AST_INT_LITERAL, buf);
    lit->decl_type = TYPE_LONG;
    lit->is_unsigned = 1;
    return lit;
}
```

### 3 — `AST_SIZEOF_TYPE` folding in `optimize_expr`

Insert the following block **after** the "Conditional expression folding" block
(after `return keep;` on ~line 401) and **before** the `return node;` that ends
`optimize_expr`:

```c
    /* sizeof(type) folding: AST_SIZEOF_TYPE is always a compile-time constant.
       All type information (decl_type, full_type) is set by the parser. */
    if (node->type == AST_SIZEOF_TYPE) {
        int sz;
        if ((node->decl_type == TYPE_STRUCT || node->decl_type == TYPE_UNION ||
                node->decl_type == TYPE_ARRAY) && node->full_type) {
            sz = node->full_type->size;
        } else {
            sz = sizeof_scalar_size(node->decl_type);
        }
        ast_free(node);
        return make_sizeof_literal(sz);
    }
```

### 4 — `AST_SIZEOF_EXPR` folding (literal operands) in `optimize_expr`

Insert the following block immediately after the `AST_SIZEOF_TYPE` block above
(still before `return node;`):

```c
    /* sizeof(expr) folding for operands whose size the optimizer can determine
       without symbol-table access.  All other cases fall through to codegen. */
    if (node->type == AST_SIZEOF_EXPR && node->child_count == 1 &&
            node->children[0] != NULL) {
        ASTNode *child = node->children[0];
        /* sizeof("literal") = byte_length + 1 (includes null terminator). */
        if (child->type == AST_STRING_LITERAL) {
            int sz = child->byte_length + 1;
            ast_free(node);
            return make_sizeof_literal(sz);
        }
        /* sizeof(integer-literal): size from suffix-determined decl_type. */
        if (child->type == AST_INT_LITERAL) {
            int sz = sizeof_scalar_size(child->decl_type);
            ast_free(node);
            return make_sizeof_literal(sz);
        }
        /* sizeof(char-literal): character constants have type int in C99. */
        if (child->type == AST_CHAR_LITERAL) {
            ast_free(node);
            return make_sizeof_literal(4);
        }
        /* Variable references and complex expressions: leave for codegen. */
    }
```

---

## Memory management

Both blocks follow the same idiom used in stages 145 and 149:

1. Compute the size.
2. Call `ast_free(node)` to free the sizeof node and all its children (the type
   information stored in `node->full_type` is NOT freed here — `full_type` is
   owned by the type-table that lives in the `Parser`, not by the AST node).
3. Return `make_sizeof_literal(sz)`, a freshly allocated `AST_INT_LITERAL`.

Because `make_sizeof_literal` allocates a new node, there is no aliasing issue
between the freed node and the returned literal.

**`full_type` ownership note** — `ast_free` recurses into `node->children` but
does NOT free `node->full_type`.  The existing `ast_free` implementation does
not own `full_type` (it is a pointer into the parser's struct/typedef tables).
No change to `ast_free` is required.

---

## Bootstrap caveats

`src/optimize.c` must compile cleanly under the C0 compiler (previous-stage
binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations (`sz`, `lit`, `buf`) must appear at the top of each block before
  any executable statements.
- `snprintf` is in `<stdio.h>`, already included.
- `ast_new`, `ast_free`, `AST_INT_LITERAL`, `AST_SIZEOF_TYPE`,
  `AST_SIZEOF_EXPR`, `AST_STRING_LITERAL`, `AST_CHAR_LITERAL`,
  `TYPE_LONG`, `TYPE_BOOL`, `TYPE_CHAR`, `TYPE_SHORT`,
  `TYPE_INT`, `TYPE_LONG`, `TYPE_LONG_LONG`, `TYPE_UNSIGNED_LONG_LONG`,
  `TYPE_FLOAT`, `TYPE_DOUBLE`, `TYPE_POINTER`, `TYPE_STRUCT`, `TYPE_UNION`,
  `TYPE_ARRAY` are all declared in `"ast.h"` (which transitively includes
  `"type.h"`), already included.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
are unaffected.

### test_sizeof_type_fold

`sizeof(type)` folded and composed with stage-143 binary folding, proving the
literal is visible to the optimizer.  A test that merely called `sizeof(int)`
and printed it would pass even without optimizer folding (codegen emits the
correct value either way); these expressions only produce the right result if
the sizeof literal participates in constant folding:

```c
/* test/integration/test_sizeof_type_fold/test_sizeof_type_fold.c */
#include <stdio.h>
int main(void) {
    /* sizeof(int) -> 4; 4 * 2 -> 8 (stage 143) */
    int a = sizeof(int) * 2;
    /* sizeof(long) -> 8; sizeof(char) -> 1; 8 - 1 -> 7 (stage 143) */
    int b = sizeof(long) - sizeof(char);
    /* sizeof(void *) -> 8; 8 / 2 -> 4 (stage 143) */
    int c = sizeof(void *) / 2;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `8 7 4`

### test_sizeof_expr_fold

`sizeof` in arithmetic expressions — requires fold for stage-143 to compose:

```c
/* test/integration/test_sizeof_expr_fold/test_sizeof_expr_fold.c */
#include <stdio.h>
int main(void) {
    /* sizeof(int) * 8 -> 4 * 8 -> 32 (stage 151 then stage 143) */
    int a = sizeof(int) * 8;
    /* sizeof(long) + sizeof(int) -> 8 + 4 -> 12 */
    int b = sizeof(long) + sizeof(int);
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `32 12`

### test_sizeof_dead_branch

`sizeof` in a condition — requires fold for stage-150 to eliminate dead branch:

```c
/* test/integration/test_sizeof_dead_branch/test_sizeof_dead_branch.c */
#include <stdio.h>
int main(void) {
    int a = 0;
    /* sizeof(long) -> 8; 8 == 8 -> 1 (stage 143); if(1) keeps then-branch */
    if (sizeof(long) == 8) { a = 10; } else { a = 99; }
    int b = 0;
    /* sizeof(int) -> 4; 4 == 8 -> 0 (stage 143); if(0) keeps else-branch */
    if (sizeof(int) == 8) { b = 99; } else { b = 20; }
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `10 20`

### test_sizeof_string_fold

`sizeof(string-literal)` folded at optimizer time:

```c
/* test/integration/test_sizeof_string_fold/test_sizeof_string_fold.c */
#include <stdio.h>
int main(void) {
    /* sizeof("hi") == 3 (2 chars + null) */
    int a = sizeof("hi");
    /* sizeof("") == 1 (null only) */
    int b = sizeof("");
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `3 1`

### test_sizeof_struct_fold

`sizeof` of a struct type folded to the layout size:

```c
/* test/integration/test_sizeof_struct_fold/test_sizeof_struct_fold.c */
#include <stdio.h>
struct Point { int x; int y; };
int main(void) {
    /* sizeof(struct Point) -> 8 (two 4-byte ints, no padding) */
    int a = sizeof(struct Point);
    /* sizeof(int[4]) -> 16 */
    int b = sizeof(int[4]);
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `8 16`

### Regression

Run the full test suite via `./run_tests.sh`.  All existing tests must continue
to pass at both `-O0` and `-O1`.  In particular, `sizeof` of variable
expressions (e.g., `sizeof(x)` where `x` is a local variable) must still
produce the correct size via the codegen fallback path.

---

## Implementation order

1. In `src/optimize.c`, add the two static helpers (`sizeof_scalar_size`,
   `make_sizeof_literal`) before the definition of `optimize_expr`.
2. In `optimize_expr`, insert the `AST_SIZEOF_TYPE` folding block and the
   `AST_SIZEOF_EXPR` folding block immediately before the final `return node;`.
3. Update the file-top comment block to include:
   `Stage 151: sizeof constant folding -- AST_SIZEOF_TYPE/EXPR -> AST_INT_LITERAL.`
4. Build: `cmake --build build`.
5. Smoke test:
   ```sh
   printf '#include <stdio.h>\nint main(void){\n  printf("%%d %%d %%d\\n", (int)sizeof(int), (int)sizeof(long), (int)sizeof(int)*8);\n  return 0;}\n' > /tmp/szfold.c
   ./build/ccompiler -O1 /tmp/szfold.c -o /tmp/szfold.asm
   nasm -f elf64 /tmp/szfold.asm -o /tmp/szfold.o && gcc /tmp/szfold.o -o /tmp/szfold && /tmp/szfold
   ```
   Expected output: `4 8 32`
6. Add the five integration tests.
7. Run `./run_tests.sh` — all tests pass.
8. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
9. Bump `VERSION_STAGE` to `"01510000"` in `src/version.c`.

---

## Out of scope — do NOT do these in this stage

- `sizeof(variable)` — requires symbol-table lookup (`codegen_find_var`/
  `codegen_find_global`); the optimizer does not have access to these tables.
  Handled by the existing codegen fallback.  Will become foldable after the
  constant-propagation stage annotates variable types onto `AST_VAR_REF` nodes.
- `sizeof(complex-expr)` — binary/unary expressions, function calls, member
  accesses, etc.; type resolution requires full codegen context.  Handled by
  the existing codegen fallback (`sizeof_type_of_expr`).
- `sizeof(double)` discrepancy — the existing codegen switch does not list
  `TYPE_DOUBLE` explicitly; it falls through to `default: sz = 4`, which is
  incorrect.  The optimizer helper `sizeof_scalar_size` adds an explicit
  `TYPE_DOUBLE: return 8` case, correcting this for `-O1`.  Fixing the codegen
  path is out of scope for this stage.
- Float/double `sizeof(expr)` literals — `sizeof(3.14)` and `sizeof(3.14f)`;
  `AST_FLOAT_LITERAL` nodes with FP type are not handled by this stage.
- Fold through casts — `sizeof((int)x)` returns `sizeof(int)` regardless of `x`;
  requires the cast-folding stage.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "sizeof constant folding" item as
  complete (`[x]`), annotating Stage 151; add a `## Stage 151` section after
  `## Stage 150`.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-151 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01510000"`).
