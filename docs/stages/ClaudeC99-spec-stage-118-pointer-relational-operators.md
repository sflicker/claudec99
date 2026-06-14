# ClaudeC99 Stage 118 — Pointer Relational Operators

## Goal

Add support for the four relational comparison operators (`<`, `<=`, `>`, `>=`)
when applied to pointer operands.  The compiler already handles pointer equality
(`==`, `!=`) and pointer arithmetic (`+`, `-`, pointer difference), but
relational comparison of two pointers currently hits the catch-all
`compile_error` branch and rejects the program:

```
error: operator '<' not supported on pointer operands
```

This blocks every common pointer-walk idiom:

```c
while (p < end) { ... p++; }
```

This is a **codegen-only stage**.  No tokenizer, parser, or AST changes are
needed: the parser already produces `AST_BINARY_OP` nodes with `node->value`
set to `"<"`, `"<="`, `">"`, or `">="` for these operators.

---

## Root-cause analysis

All four missing operators are handled in `codegen_expression()` inside the
`AST_BINARY_OP` case, around line 4312–4346 of `src/codegen.c`.

```c
if (lt == TYPE_POINTER || rt == TYPE_POINTER) {
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
        is_pointer_cmp = 1;
        common = TYPE_LONG;
    } else if (strcmp(op, "+") == 0) {
        ...
    } else if (strcmp(op, "-") == 0) {
        ...
    } else {
        compile_error(                              /* <, <=, >, >= land here */
                "error: operator '%s' not supported on pointer operands\n",
                op);
    }
}
```

When either operand is `TYPE_POINTER` and the operator is `<`, `<=`, `>`, or
`>=`, the code falls through to the `compile_error` branch.

There are two further issues that must be fixed together:

### Issue A — `setcc` instruction uses signed variants for pointer comparisons

After both operands are evaluated, the comparison block at line ~4577 selects a
`setcc` instruction based on `op_is_unsigned`:

```c
const char *setcc = NULL;
if      (strcmp(op, "==") == 0) setcc = "sete";
else if (strcmp(op, "!=") == 0) setcc = "setne";
else if (op_is_unsigned) {
    if      (strcmp(op, "<")  == 0) setcc = "setb";   /* unsigned */
    ...
} else {
    if      (strcmp(op, "<")  == 0) setcc = "setl";   /* signed */
    ...
}
```

`op_is_unsigned` is computed only when `!is_pointer_cmp` (line ~4492), so it
stays `0` for all pointer comparisons.  If we simply extend `is_pointer_cmp` to
cover `<`/`<=`/`>`/`>=`, the code would correctly reach the comparison block
but emit `setl`/`setle`/`setg`/`setge` (signed).  Pointer addresses are
unsigned 64-bit values, so this produces wrong results when the high bit of
either address is set (all heap/stack addresses on x86-64 Linux have the high
bit clear today, but the ABI makes no such guarantee).  The fix is to use the
unsigned `setb`/`setbe`/`seta`/`setae` variants for all pointer comparisons.

### Issue B — Validation block does not guard against `pointer < integer`

The existing validation block (lines ~4377–4398) handles the two already-
supported pointer equality operators.  When `is_pointer_cmp` is true, it allows
`pointer == 0` (the null pointer constant) but rejects `pointer == nonzero_int`.
For the relational operators, C99 does not define the result of comparing a
pointer with any integer (including 0); only `==` and `!=` against the null
pointer constant are defined.  The validation block must be extended to emit a
diagnostic when a relational pointer comparison has a non-pointer operand.

---

## Task 1 — Extend `is_pointer_cmp` to include `<`, `<=`, `>`, `>=`

In `codegen_expression()`, inside the `if (lt == TYPE_POINTER || rt ==
TYPE_POINTER)` block (~line 4316):

**Before**:

```c
        if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
            is_pointer_cmp = 1;
            common = TYPE_LONG;
        } else if (strcmp(op, "+") == 0) {
```

**After**:

```c
        if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
            strcmp(op, "<")  == 0 || strcmp(op, "<=") == 0 ||
            strcmp(op, ">")  == 0 || strcmp(op, ">=") == 0) {
            is_pointer_cmp = 1;
            common = TYPE_LONG;
        } else if (strcmp(op, "+") == 0) {
```

---

## Task 2 — Extend validation to reject `pointer < integer`

In the `if (is_pointer_cmp)` validation block (~line 4377):

**Before**:

```c
        if (is_pointer_cmp) {
            ASTNode *lhs = node->children[0];
            ASTNode *rhs = node->children[1];
            int lhs_ptr = (lhs->result_type == TYPE_POINTER);
            int rhs_ptr = (rhs->result_type == TYPE_POINTER);
            if (lhs_ptr && rhs_ptr) {
                if (!pointer_types_equal(lhs->full_type, rhs->full_type)) {
                    compile_error(
                            "error: incompatible pointer types in comparison\n");
                }
            } else if (lhs_ptr && !rhs_ptr) {
                if (!is_null_pointer_constant(rhs)) {
                    compile_error(
                            "error: comparing pointer with non zero integer\n");
                }
            } else if (!lhs_ptr && rhs_ptr) {
                if (!is_null_pointer_constant(lhs)) {
                    compile_error(
                            "error: comparing pointer with non zero integer\n");
                }
            }
        }
```

**After**:

```c
        if (is_pointer_cmp) {
            ASTNode *lhs = node->children[0];
            ASTNode *rhs = node->children[1];
            int lhs_ptr = (lhs->result_type == TYPE_POINTER);
            int rhs_ptr = (rhs->result_type == TYPE_POINTER);
            int is_relcmp = (strcmp(op, "<")  == 0 || strcmp(op, "<=") == 0 ||
                             strcmp(op, ">")  == 0 || strcmp(op, ">=") == 0);
            if (lhs_ptr && rhs_ptr) {
                if (!pointer_types_equal(lhs->full_type, rhs->full_type)) {
                    compile_error(
                            "error: incompatible pointer types in comparison\n");
                }
            } else if (is_relcmp) {
                /* Relational operators require both operands to be pointers. */
                compile_error(
                        "error: relational comparison requires two pointer operands\n");
            } else if (lhs_ptr && !rhs_ptr) {
                if (!is_null_pointer_constant(rhs)) {
                    compile_error(
                            "error: comparing pointer with non zero integer\n");
                }
            } else if (!lhs_ptr && rhs_ptr) {
                if (!is_null_pointer_constant(lhs)) {
                    compile_error(
                            "error: comparing pointer with non zero integer\n");
                }
            }
        }
```

The new `is_relcmp` branch fires before the null-constant checks, so
`pointer < 0` is rejected even though `pointer == 0` is allowed.

---

## Task 3 — Use unsigned `setcc` for pointer relational comparisons

In the `setcc` selection block (~line 4577):

**Before**:

```c
            const char *setcc = NULL;
            if      (strcmp(op, "==") == 0) setcc = "sete";
            else if (strcmp(op, "!=") == 0) setcc = "setne";
            else if (op_is_unsigned) {
                if      (strcmp(op, "<")  == 0) setcc = "setb";
                else if (strcmp(op, "<=") == 0) setcc = "setbe";
                else if (strcmp(op, ">")  == 0) setcc = "seta";
                else if (strcmp(op, ">=") == 0) setcc = "setae";
            } else {
                if      (strcmp(op, "<")  == 0) setcc = "setl";
                else if (strcmp(op, "<=") == 0) setcc = "setle";
                else if (strcmp(op, ">")  == 0) setcc = "setg";
                else if (strcmp(op, ">=") == 0) setcc = "setge";
            }
```

**After**:

```c
            const char *setcc = NULL;
            if      (strcmp(op, "==") == 0) setcc = "sete";
            else if (strcmp(op, "!=") == 0) setcc = "setne";
            else if (op_is_unsigned || is_pointer_cmp) {
                /* Pointer addresses are unsigned; relational pointer
                 * comparisons must use the unsigned setcc variants. */
                if      (strcmp(op, "<")  == 0) setcc = "setb";
                else if (strcmp(op, "<=") == 0) setcc = "setbe";
                else if (strcmp(op, ">")  == 0) setcc = "seta";
                else if (strcmp(op, ">=") == 0) setcc = "setae";
            } else {
                if      (strcmp(op, "<")  == 0) setcc = "setl";
                else if (strcmp(op, "<=") == 0) setcc = "setle";
                else if (strcmp(op, ">")  == 0) setcc = "setg";
                else if (strcmp(op, ">=") == 0) setcc = "setge";
            }
```

The `|| is_pointer_cmp` addition does not affect `sete` or `setne` (already
handled in the first two arms), and it has no effect on existing integer
comparisons (where `is_pointer_cmp` is always `0`).

---

## Task 4 — `src/version.c`

Bump `VERSION_STAGE` to `"01180000"`.

---

## Implementation order

1. Apply Task 1 (extend `is_pointer_cmp`).
2. Apply Task 2 (extend validation block).
3. Apply Task 3 (unsigned `setcc` for pointer comparisons).
4. Build (`cmake --build build`).
5. Quick smoke test: compile and run a `while (p < end)` program by hand.
6. Run `test/valid/run_tests.sh` — all must pass.
7. Add tests (see below); run full suite (`./test/run_all_tests.sh`).
8. Bump `src/version.c`.
9. Self-host: `./build.sh --mode=self-host` — all three passes must pass.
10. Update `docs/self-compilation-report.md`.
11. Commit.
12. Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.

---

## Out of scope — do NOT do these in this stage

- **`void *` relational comparison** — C99 does not define `<`/`>`/etc. on
  `void *`.  The current `pointer_types_equal` check will correctly reject
  mismatched types.  Two `void *` operands would pass type equality and produce
  a comparison; this is an edge case not tested here and can be addressed in a
  later diagnostic stage.
- **Function pointer relational comparison** — undefined behavior in C99; no
  test added here.
- **Three-way comparison patterns** — no need for any new AST nodes or
  intrinsics; the operator-by-operator `setcc` approach handles every pattern.
- **Pointer comparison in constant expressions** — `eval_const_expr()` does not
  need to evaluate pointer comparisons; only runtime comparisons are in scope.

---

## Bootstrap caveats

The compiler's own source (`src/*.c`) uses pointer relational comparisons in
several places (e.g. `p < end` style loops in string-processing helpers and
`vec_get` bounds checks).  These comparisons are integer-or-pointer expressions
that the compiler currently handles correctly at the integer level.  After this
fix, such expressions may be re-routed through the new `is_pointer_cmp` path;
the generated code must be equivalent.  The self-host bootstrap will confirm
correctness: if C1 and C2 produce the same output as C0 on all 1863 existing
tests, the fix is safe.

---

## Tests

### Valid tests — place in `test/valid/pointers/`

#### Smoke test — `<` with adjacent array elements

**`test/valid/pointers/test_ptr_lt__1.c`**
```c
int main(void) {
    int arr[2];
    int *lo = arr;
    int *hi = arr + 1;
    return (lo < hi) ? 1 : 0;   /* 1 */
}
```
Expected exit: 1

#### `<=` — equal pointers

**`test/valid/pointers/test_ptr_le_eq__1.c`**
```c
int main(void) {
    int x;
    int *p = &x;
    int *q = &x;
    return (p <= q) ? 1 : 0;   /* 1 — same address */
}
```
Expected exit: 1

#### `>` — reversed comparison

**`test/valid/pointers/test_ptr_gt__1.c`**
```c
int main(void) {
    int arr[2];
    int *lo = arr;
    int *hi = arr + 1;
    return (hi > lo) ? 1 : 0;   /* 1 */
}
```
Expected exit: 1

#### `>=` — equal pointers

**`test/valid/pointers/test_ptr_ge_eq__1.c`**
```c
int main(void) {
    int x;
    int *p = &x;
    int *q = &x;
    return (p >= q) ? 1 : 0;   /* 1 — same address */
}
```
Expected exit: 1

#### Pointer-walk loop — canonical idiom

**`test/valid/pointers/test_ptr_walk_lt__15.c`**
```c
int main(void) {
    int arr[5];
    int i;
    for (i = 0; i < 5; i++) arr[i] = i + 1;   /* 1 2 3 4 5 */
    int *p = arr;
    int *end = arr + 5;
    int sum = 0;
    while (p < end) {
        sum += *p;
        p++;
    }
    return sum;   /* 15 */
}
```
Expected exit: 15

#### Pointer-walk loop with `<=`

**`test/valid/pointers/test_ptr_walk_le__5.c`**
```c
int main(void) {
    int arr[5];
    int i;
    for (i = 0; i < 5; i++) arr[i] = i + 1;
    int *first = arr;
    int *last  = arr + 4;   /* points at the last element, not one-past */
    int count = 0;
    int *p = first;
    while (p <= last) {
        count++;
        p++;
    }
    return count;   /* 5 */
}
```
Expected exit: 5

#### Mixed `<` and `>` in a find-bounds function

**`test/valid/pointers/test_ptr_bounds__42.c`**
```c
static int data[6] = {10, 20, 30, 40, 50, 60};

int main(void) {
    int *lo = data;
    int *hi = data + 5;
    int *mid = data + 3;
    /* mid > lo and mid < hi */
    if (mid > lo && mid < hi)
        return *mid + 2;   /* 40 + 2 = 42 */
    return 0;
}
```
Expected exit: 42

### Invalid tests — place in `test/invalid/pointers/`

#### Relational comparison of pointer with integer (0)

**`test/invalid/pointers/test_ptr_lt_int__relational_comparison_requires.c`**
```c
int main(void) {
    int x = 0;
    int *p = &x;
    return (p < 0);
}
```
Expected error: `relational comparison requires`

#### Relational comparison of incompatible pointer types

**`test/invalid/pointers/test_ptr_lt_incompatible__incompatible_pointer_types.c`**
```c
int main(void) {
    int  x = 0;
    float y = 0.0f;
    int   *p = &x;
    float *q = &y;
    return (p < q);
}
```
Expected error: `incompatible pointer types`

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — add pointer relational operators to the pointer/arithmetic
  bullet; update test totals.
- **`docs/self-compilation-report.md`** — add stage-118 self-host result.
- Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.
