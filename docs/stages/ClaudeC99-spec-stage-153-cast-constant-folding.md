# ClaudeC99 Stage 153 — Cast Constant Folding

## Goal

Fold `AST_CAST` nodes whose operand has been reduced to `AST_INT_LITERAL` into a
fresh `AST_INT_LITERAL` carrying the cast's target type, provided the integer
value is preserved by the cast (i.e., the source value fits in the target type
without truncation or sign change).

This stage touches only `src/optimize.c`; no other source files change.  All
folding is gated behind `-O1`; the `-O0` default path is unaffected.

---

## Background

A C cast expression such as `(int)expr` is represented in the AST as an
`AST_CAST` node with `decl_type` set to the target type and `children[0]` set
to the operand.  When the operand resolves to a compile-time constant, the
cast result is also a compile-time constant, but the optimizer currently
cannot see it:

```c
int a = (int)sizeof(long);        /* (int)8 not literal; stage 143 can't fold 8 + 1 */
const long N = 5;
int b = (int)N + 3;               /* (int)5 not literal; stage 143 can't fold 5 + 3 */
if ((int)sizeof(long) == 8) ...   /* condition not literal; stage 150 can't eliminate */
```

After this stage, `optimize_expr` folds `AST_CAST` of `AST_INT_LITERAL` (target
is a scalar integer type, value unchanged) to a fresh `AST_INT_LITERAL`.
Because the optimizer is a bottom-up tree walk, child expressions are already
fully folded before the parent `AST_CAST` is inspected, so the chain
`sizeof → literal → cast → literal → binary fold` works in a single pass.

Note: parenthesized expressions `(expr)` do **not** produce an `AST_PAREN`
node in this compiler — the parser consumes the parentheses and returns the
inner expression directly.  "Fold through parentheses" in the checklist title
refers to C's cast syntax `(type)expr`, which uses parentheses around the type
name.

Related prior work:

- **Stage 142** — optimizer skeleton: `optimize_expr` / `optimize_stmt` /
  `optimize_translation_unit`.
- **Stage 143** — constant integer binary folding; depends on both operands
  being `AST_INT_LITERAL`.
- **Stage 150** — dead-branch elimination; depends on the condition being
  `AST_INT_LITERAL`.
- **Stage 151** — sizeof constant folding; same free/replace idiom used here.
- **Stage 152** — const propagation for const scalar locals; produces
  `AST_INT_LITERAL` nodes that may immediately flow into cast operands.

---

## Eligibility rules

An `AST_CAST` node is folded if and only if all of the following hold after
the bottom-up recursion into children:

| Criterion | How checked |
|-----------|-------------|
| Node type is a cast | `node->type == AST_CAST` |
| Operand is a literal | `node->child_count == 1 && node->children[0]->type == AST_INT_LITERAL` |
| Target is scalar integer | `is_scalar_int_type(node->decl_type)` returns 1 (reuses stage-152 helper) |
| Value fits in target type | `cast_value_safe(val, node->decl_type, node->is_unsigned)` returns 1 |

The fourth check verifies that the integer value of the operand literal is
numerically identical before and after the cast.  When the value would be
truncated or sign-changed, the node is left unchanged and codegen handles it
as before.

**Value-safe ranges** (helper `cast_value_safe`):

| Target TypeKind | `is_unsigned` | Safe when |
|-----------------|---------------|-----------|
| `TYPE_BOOL` | 0 | `val == 0 || val == 1` |
| `TYPE_CHAR` | 0 | `-128 ≤ val ≤ 127` |
| `TYPE_CHAR` | 1 | `0 ≤ val ≤ 255` |
| `TYPE_SHORT` | 0 | `-32768 ≤ val ≤ 32767` |
| `TYPE_SHORT` | 1 | `0 ≤ val ≤ 65535` |
| `TYPE_INT` | 0 | `-2147483648 ≤ val ≤ 2147483647` |
| `TYPE_INT` | 1 | `0 ≤ val ≤ 4294967295` |
| `TYPE_LONG` | 0 | always (signed 64-bit; `long` stores value exactly) |
| `TYPE_LONG` | 1 | `val ≥ 0` |
| `TYPE_LONG_LONG` | 0 | always |
| `TYPE_LONG_LONG` | 1 | `val ≥ 0` |
| `TYPE_UNSIGNED_LONG_LONG` | — | `val ≥ 0` |

`TYPE_UNSIGNED_LONG_LONG` has its own TypeKind (it is not `TYPE_LONG_LONG` +
`is_unsigned=1`); the unsigned check is the same — `val ≥ 0` — because the
optimizer stores all literal values as `long`, whose maximum is 2^63-1, which
is less than 2^64-1, so all non-negative `long` values fit in unsigned 64-bit.

**Not eligible** (do not fold):

- Casts to non-integer types: `TYPE_FLOAT`, `TYPE_DOUBLE`, `TYPE_POINTER`,
  `TYPE_STRUCT`, `TYPE_UNION`, `TYPE_ARRAY`.
- Casts where the operand is not `AST_INT_LITERAL` (variable references,
  floats, function calls, etc.) — leave for codegen.
- Casts where the value does not fit in the target type (e.g., `(char)300` is
  implementation-defined and codegen handles the truncation; `(unsigned)(-1)`
  has a well-defined but different value — codegen emits the correct result).

---

## Already handled by prior stages (do NOT re-implement)

| Pattern | Result | Stage |
|---------|--------|-------|
| `const_a OP const_b` | folded literal | 143 |
| `OP const` | folded literal | 144 |
| `x && 0`, `0 && x` | `0` | 147 |
| `x \|\| 1`, `1 \|\| x` | `1` | 147 |
| `-(-x)` | `x` | 148 |
| `const ? T : F` | selected branch | 149 |
| `if/while/for (0)` | dead-branch drop | 150 |
| `sizeof(type/expr)` | folded literal | 151 |
| `const`-scalar-local `VAR_REF` | propagated literal | 152 |

---

## Semantic notes

**Result type** — The new `AST_INT_LITERAL` carries `decl_type` and
`is_unsigned` from the `AST_CAST` node (the declared target type of the
cast), not from the source operand.  For example, `(long)5` produces a
literal with `decl_type = TYPE_LONG, is_unsigned = 0`.  This is correct
because the C type of `(T)expr` is always `T`.

**Value string** — The formatted value string in the new literal is
`snprintf`-ed from the `long val` read from the source literal.  Since the
safe-range check guarantees `val` is unchanged, the printed decimal is the
same number, just with a potentially different type annotation.  No decimal
rounding or truncation occurs.

**`ast_free` does not free `node->value`** — Confirmed in stage 151/152
notes: `ast_free` frees `node->children` and the node itself, but not the
`value` string.  When `ast_free(node)` is called on the `AST_CAST`, the
child `AST_INT_LITERAL` is freed through the children array, but its `value`
string is not freed (it was either a string literal or a `util_strdup`
allocation that the optimizer never frees).  The `val` read via `strtol`
before the `ast_free` call is a `long` local, so there is no use-after-free.

---

## Task — `src/optimize.c`: add cast folding

### 1 — Static helper: `cast_value_safe`

Insert the following helper **before** `optimize_expr` (after the existing
`const_prop_lookup` helper):

```c
/* Return 1 if val can be represented exactly in integer type (target, unsigned).
   Used to gate AST_CAST folding: only fold when the numeric value is preserved. */
static int cast_value_safe(long val, TypeKind target, int target_unsigned) {
    switch (target) {
    case TYPE_BOOL:
        return (val == 0L || val == 1L);
    case TYPE_CHAR:
        if (target_unsigned) return (val >= 0L && val <= 255L);
        return (val >= -128L && val <= 127L);
    case TYPE_SHORT:
        if (target_unsigned) return (val >= 0L && val <= 65535L);
        return (val >= -32768L && val <= 32767L);
    case TYPE_INT:
        if (target_unsigned) return (val >= 0L && val <= 4294967295L);
        return (val >= -2147483648L && val <= 2147483647L);
    case TYPE_LONG:
    case TYPE_LONG_LONG:
        if (target_unsigned) return (val >= 0L);
        return 1; /* signed 64-bit: long stores value exactly */
    case TYPE_UNSIGNED_LONG_LONG:
        return (val >= 0L);
    default:
        return 0;
    }
}
```

### 2 — `AST_CAST` folding in `optimize_expr`

Insert the following block immediately **after** the `AST_SIZEOF_EXPR` block
(which ends with `/* Variable references and complex expressions: leave for
codegen. */}`) and **before** the `AST_VAR_REF` const-propagation block:

```c
    /* Cast constant folding: (integer_type) integer_literal -> integer_literal
       with target type, provided the value is preserved (fits in target type).
       The bottom-up walk ensures children[0] is already folded before this
       fires, so sizeof/const-prop results flow through casts in one pass. */
    if (node->type == AST_CAST &&
            node->child_count == 1 &&
            node->children[0] != NULL &&
            node->children[0]->type == AST_INT_LITERAL &&
            is_scalar_int_type(node->decl_type)) {
        long val = strtol(node->children[0]->value, NULL, 0);
        if (cast_value_safe(val, node->decl_type, node->is_unsigned)) {
            char buf[32];
            ASTNode *lit;
            TypeKind dst_type     = node->decl_type;
            int      dst_unsigned = node->is_unsigned;
            snprintf(buf, sizeof(buf), "%ld", val);
            ast_free(node); /* frees AST_CAST and its INT_LITERAL child */
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type   = dst_type;
            lit->is_unsigned = dst_unsigned;
            return lit;
        }
    }
```

### 3 — Update file-top comment block

Add to the existing comment at the top of `optimize.c`:

```
 * Stage 153: cast constant folding -- AST_CAST of AST_INT_LITERAL to scalar
 *            integer type replaced with a fresh AST_INT_LITERAL of target type,
 *            provided the numeric value is preserved (fits in target type).
```

---

## Memory management

The pattern mirrors stages 151 and 152:

1. Read `val` from `node->children[0]->value` via `strtol` before freeing.
2. Read `dst_type` and `dst_unsigned` from `node` before freeing.
3. `snprintf(buf, ...)` into a stack buffer.
4. `ast_free(node)` — frees the `AST_CAST` node, its `children` array, and
   the child `AST_INT_LITERAL` (recursively); `value` strings are not freed.
5. `ast_new(AST_INT_LITERAL, util_strdup(buf))` — allocates a fresh node whose
   `value` is a heap copy of `buf`.

No aliasing issue exists between the freed cast node and the returned literal.

---

## Bootstrap caveats

`src/optimize.c` must compile cleanly under the C0 compiler (previous-stage
binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- Declarations (`val`, `buf`, `lit`, `dst_type`, `dst_unsigned`) must appear
  at the top of each block before any executable statements.
- `strtol` is in `<stdlib.h>`, already included.
- `snprintf` is in `<stdio.h>`, already included.
- `util_strdup` is declared in `"util.h"`, already included.
- `ast_new`, `ast_free`, `AST_CAST`, `AST_INT_LITERAL`, `TYPE_BOOL`,
  `TYPE_CHAR`, `TYPE_SHORT`, `TYPE_INT`, `TYPE_LONG`, `TYPE_LONG_LONG`,
  `TYPE_UNSIGNED_LONG_LONG` are declared in `"ast.h"` (which transitively
  includes `"type.h"`), already included.
- `is_scalar_int_type` is the stage-152 static helper already in this file;
  reuse it without redeclaration.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O1` in their `.cflags` file.  Existing tests
(including all `-O0` tests) are unaffected.

### test_cast_fold_basic

Direct cast of an integer literal — produces a literal of the target type:

```c
/* test/integration/test_cast_fold_basic/test_cast_fold_basic.c */
#include <stdio.h>
int main(void) {
    int  a = (int)42L;       /* (int)42L -> 42 (int) */
    long b = (long)7;        /* (long)7  -> 7L (long) */
    int  c = (int)100LL;     /* (int)100LL -> 100 (int) */
    printf("%d %ld %d\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `42 7 100`

### test_cast_fold_sizeof

Cast of a `sizeof` result — requires stage-151 to fold `sizeof` first, then
stage-153 to fold the cast, then stage-143 to fold the binary expression:

```c
/* test/integration/test_cast_fold_sizeof/test_cast_fold_sizeof.c */
#include <stdio.h>
int main(void) {
    /* sizeof(long)->8L; (int)8L->8; 8+1->9 (stage 143) */
    int a = (int)sizeof(long) + 1;
    /* sizeof(int)->4L; (int)4L->4; 4*2->8 (stage 143) */
    int b = (int)sizeof(int) * 2;
    /* sizeof(char)->1L; (long)1L->1L; 1L+7L->8L (stage 143) */
    long c = (long)sizeof(char) + 7L;
    printf("%d %d %ld\n", a, b, c);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `9 8 8`

### test_cast_fold_const_prop

Cast of a const-propagated variable — requires stage-152 to substitute the
`VAR_REF` first, producing a literal that stage-153 then folds through the cast:

```c
/* test/integration/test_cast_fold_const_prop/test_cast_fold_const_prop.c */
#include <stdio.h>
int main(void) {
    const long N = 100;
    int x = (int)N;          /* N->100L (stage 152); (int)100L->100 (stage 153) */
    int y = (int)N + 5;      /* -> 100 + 5 -> 105 (stage 143) */
    printf("%d %d\n", x, y);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `100 105`

### test_cast_fold_dead_branch

Cast in a condition — stage-151 folds `sizeof`, stage-153 folds the cast,
stage-143 folds the comparison, stage-150 eliminates the dead branch:

```c
/* test/integration/test_cast_fold_dead_branch/test_cast_fold_dead_branch.c */
#include <stdio.h>
int main(void) {
    int a = 0;
    /* (int)sizeof(long)==8 -> 8==8 -> 1; if(1) keeps then-branch */
    if ((int)sizeof(long) == 8) { a = 1; } else { a = 99; }
    int b = 0;
    /* (int)sizeof(int)==8 -> 4==8 -> 0; if(0) keeps else-branch */
    if ((int)sizeof(int) == 8) { b = 99; } else { b = 2; }
    printf("%d %d\n", a, b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `1 2`

### test_cast_fold_unsafe

Casts that change the value must NOT be folded — codegen must handle them
and produce the correct (C-defined or implementation-defined) result:

```c
/* test/integration/test_cast_fold_unsafe/test_cast_fold_unsafe.c */
#include <stdio.h>
int main(void) {
    /* (char)300: value 300 does not fit in signed char (-128..127);
       optimizer leaves the cast; codegen truncates to 44 */
    char a = (char)300;
    /* (unsigned char)(-1): value -1 does not fit in 0..255 unsigned byte;
       codegen produces 255 */
    unsigned char b = (unsigned char)(-1);
    printf("%d %d\n", (int)a, (int)b);
    return 0;
}
```

`.cflags`: `-O1`
`.expected`: `44 255`

### Regression

Run the full test suite via `./run_tests.sh`.  All existing tests must
continue to pass at both `-O0` and `-O1`.  In particular:

- Casts to `float` / `double` / pointer types must NOT be folded.
- Casts of non-literal operands (variable references, binary expressions,
  function calls) must NOT be folded.
- Casts that change the value (truncation, unsigned wrap) must NOT be folded
  and must still produce the correct output via codegen.

---

## Implementation order

1. In `src/optimize.c`, add the `cast_value_safe` static helper before the
   definition of `optimize_expr` (after `const_prop_lookup`).
2. In `optimize_expr`, insert the `AST_CAST` folding block between the
   `AST_SIZEOF_EXPR` block and the `AST_VAR_REF` const-propagation block,
   immediately before the final `return node;`.
3. Update the file-top comment block.
4. Build: `cmake --build build`.
5. Smoke test:
   ```sh
   printf '#include <stdio.h>\nint main(void){\n  int a=(int)sizeof(long)+1;\n  printf("%%d\\n",a);\n  return 0;}\n' > /tmp/castfold.c
   ./build/ccompiler -O1 /tmp/castfold.c -o /tmp/castfold.asm
   nasm -f elf64 /tmp/castfold.asm -o /tmp/castfold.o && gcc /tmp/castfold.o -o /tmp/castfold && /tmp/castfold
   ```
   Expected output: `9`
6. Add the five integration tests.
7. Run `./run_tests.sh` — all tests pass.
8. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
9. Bump `VERSION_STAGE` to `"01530000"` in `src/version.c`.

---

## Out of scope — do NOT do these in this stage

- **Casts to `float` / `double`** — `AST_FLOAT_LITERAL` is not `AST_INT_LITERAL`;
  the result type is not handled by `is_scalar_int_type`.  Float conversions
  require FP arithmetic that the optimizer does not perform.
- **Casts to pointer types** — `TYPE_POINTER` is excluded by `is_scalar_int_type`.
  Integer-to-pointer casts are implementation-defined; codegen handles them.
- **Casts where the value changes** (truncating, sign-changing, unsigned-wrapping
  casts) — excluded by `cast_value_safe`.  Codegen produces the correct C result.
- **Casts of `AST_FLOAT_LITERAL` operands** — floating-point-to-integer
  conversion is non-trivial; leave for a later stage.
- **Double-cast chains `(T2)(T1)expr`** — handled automatically because the
  bottom-up walk folds the inner `AST_CAST` first (producing a literal), and
  the outer `AST_CAST` is then folded in the same pass.  No special treatment
  needed.
- **`AST_DECL_LIST` with const declarations** — out of scope since stage 152;
  not affected by this stage.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Fold through parentheses /
  `AST_CAST`" item as complete (`[x]`), annotating Stage 153; add a
  `## Stage 153` section after `## Stage 152`.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-153 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01530000"`).
