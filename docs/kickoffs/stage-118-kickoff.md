# Stage 118 Kickoff — Pointer Relational Operators

## Summary of the spec

Stage 118 adds support for the four relational comparison operators (`<`, `<=`, `>`, `>=`) when applied to pointer operands. The parser already produces `AST_BINARY_OP` nodes with these operators; the compiler currently rejects them with `compile_error`. This stage is **codegen-only** — no tokenizer, parser, or AST changes are needed.

Relational pointer comparisons are essential for common pointer-walk idioms:
```c
while (p < end) { ... p++; }
```

---

## Required tokenizer changes

**None.** The tokenizer already handles `<`, `<=`, `>`, `>=` as distinct tokens.

---

## Required parser changes

**None.** The parser already produces `AST_BINARY_OP` nodes with `node->value` set to `"<"`, `"<="`, `">"`, or `">="` for these operators.

---

## Required AST changes

**None.** The AST representation is unchanged.

---

## Required code generation changes

Three targeted edits in `src/codegen.c`:

### Task 1 — Extend `is_pointer_cmp` to include relational operators

Around line 4316, in the `if (lt == TYPE_POINTER || rt == TYPE_POINTER)` block:

```c
/* Before: only == and != are recognized */
if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
    is_pointer_cmp = 1;
    common = TYPE_LONG;
} else if (strcmp(op, "+") == 0) {

/* After: include <, <=, >, >= */
if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
    strcmp(op, "<")  == 0 || strcmp(op, "<=") == 0 ||
    strcmp(op, ">")  == 0 || strcmp(op, ">=") == 0) {
    is_pointer_cmp = 1;
    common = TYPE_LONG;
} else if (strcmp(op, "+") == 0) {
```

### Task 2 — Extend validation block to reject `pointer < integer`

Around line 4377, in the `if (is_pointer_cmp)` validation block:

Add an `is_relcmp` flag to distinguish relational operators from equality operators:

```c
int is_relcmp = (strcmp(op, "<")  == 0 || strcmp(op, "<=") == 0 ||
                 strcmp(op, ">")  == 0 || strcmp(op, ">=") == 0);
```

Insert a new check **before** the existing null-constant checks:

```c
} else if (is_relcmp) {
    /* Relational operators require both operands to be pointers. */
    compile_error(
            "error: relational comparison requires two pointer operands\n");
}
```

This ensures `pointer < 0` is rejected even though `pointer == 0` is allowed.

### Task 3 — Use unsigned `setcc` variants for pointer comparisons

Around line 4577, in the `setcc` selection block:

Change the condition from:
```c
else if (op_is_unsigned) {
```

to:
```c
else if (op_is_unsigned || is_pointer_cmp) {
    /* Pointer addresses are unsigned; relational pointer
     * comparisons must use the unsigned setcc variants. */
```

Pointer addresses are unsigned 64-bit values, so relational comparisons must use the unsigned variants (`setb`, `setbe`, `seta`, `setae`) instead of signed variants (`setl`, `setle`, `setg`, `setge`).

---

## Test plan

### Valid tests — place in `test/valid/pointers/`

1. **`test_ptr_lt__1.c`** — `<` with adjacent array elements
2. **`test_ptr_le_eq__1.c`** — `<=` with equal pointers
3. **`test_ptr_gt__1.c`** — `>` with reversed comparison
4. **`test_ptr_ge_eq__1.c`** — `>=` with equal pointers
5. **`test_ptr_walk_lt__15.c`** — canonical `while (p < end)` pointer-walk loop
6. **`test_ptr_walk_le__5.c`** — pointer-walk loop with `<=`
7. **`test_ptr_bounds__42.c`** — mixed `<` and `>` in a bounds-checking function

### Invalid tests — place in `test/invalid/pointers/`

1. **`test_ptr_lt_int__relational_comparison_requires.c`** — error: `pointer < 0` rejected
2. **`test_ptr_lt_incompatible__incompatible_pointer_types.c`** — error: incompatible pointer types in relational comparison

---

## Implementation steps

1. Apply Task 1 (extend `is_pointer_cmp`).
2. Apply Task 2 (extend validation block).
3. Apply Task 3 (unsigned `setcc` for pointer comparisons).
4. Build (`cmake --build build`).
5. Quick smoke test: compile and run a `while (p < end)` program by hand.
6. Run `test/valid/run_tests.sh` — all must pass.
7. Add all 7 valid and 2 invalid tests; run full suite (`./test/run_all_tests.sh`).
8. Bump `src/version.c` to `"01180000"`.
9. Self-host: `./build.sh --mode=self-host` — all three passes must pass.
10. Update `docs/self-compilation-report.md`.
11. Commit with Co-Authored-By trailer.

---

## Bootstrap caveats

The compiler's own source uses pointer relational comparisons in several places (e.g., `p < end` style loops, `vec_get` bounds checks). These expressions currently work at the integer level; after this fix, they may be re-routed through the new `is_pointer_cmp` path. The self-host bootstrap will confirm correctness: if C1 and C2 produce the same output as C0 on all existing tests, the fix is safe.
