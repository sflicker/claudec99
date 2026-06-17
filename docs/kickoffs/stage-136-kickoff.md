# Stage 136 Kickoff

## Summary of the Spec

Stage 136 fixes a bug in the `sizeof_type_of_expr` function in `src/codegen.c`. When `sizeof` is applied to a pointer-arithmetic binary expression (e.g., `sizeof(p + 1)` or `sizeof(arr + 0)`), the function incorrectly returns the element size (4) instead of the pointer size (8) on LP64 systems.

**Affected cases:**
- `sizeof(int_ptr + 1)` — pointer + integer → should return 8, currently returns 4
- `sizeof(arr + 0)` — array decays to pointer + integer → should return 8, currently returns 4
- `sizeof(double_ptr - 2)` — pointer - integer → should return 8, currently returns 4
- `sizeof(ptr1 - ptr2)` — pointer - pointer (ptrdiff_t) → should return 8, currently returns 4
- `sizeof("hi" + 0)` — string literal decays to pointer → should return 8, currently returns 4

All other `sizeof` contexts work correctly (direct variable references, subscript, dereference, address-of, cast, function call). The bug is limited to binary expressions whose result is a pointer type.

## Root Cause Analysis

The `sizeof_type_of_expr` function (line ~1827 in `src/codegen.c`) has two issues:

1. **Pointer/array type promotion bug:** The `AST_BINARY_OP` case calls `promote_kind` on each operand, but `promote_kind(TYPE_POINTER)` and `promote_kind(TYPE_ARRAY)` fall through to the default case, returning `TYPE_INT`. This causes pointer arithmetic to be mis-typed as integer arithmetic, producing size 4.

2. **Missing string literal case:** `sizeof_type_of_expr` has no `AST_STRING_LITERAL` case; string literals fall to `default: return TYPE_INT`, so `sizeof("hi" + 0)` returns 4 instead of 8.

## Required Tokenizer Changes

**None.** No new tokens are needed.

## Required Parser Changes

**None.** No parser modifications are required.

## Required AST Changes

**None.** No AST node type changes are needed.

## Required Code Generation Changes

### `src/codegen.c` — `sizeof_type_of_expr` function

**Change 1 — AST_BINARY_OP case (pointer arithmetic guard)**

In the `AST_BINARY_OP` case, before calling `promote_kind` and `common_arith_kind`, add a guard to check whether either operand resolves to a pointer or array type. If so, return `TYPE_POINTER` immediately. This must be inserted after the floating-point check but before the integer promotion path.

Required addition:
```c
/* Pointer arithmetic: (array|pointer) ± integer → pointer;
 * pointer - pointer → ptrdiff_t; both are size 8 on LP64. */
if (lt == TYPE_POINTER || lt == TYPE_ARRAY ||
    rt == TYPE_POINTER || rt == TYPE_ARRAY) {
    return TYPE_POINTER;
}
```

**Change 2 — AST_STRING_LITERAL case**

Add a new case to handle string literals in expression contexts:
```c
case AST_STRING_LITERAL:
    return TYPE_POINTER;
```

String literals have type `char[N]`; in any expression context other than direct `sizeof` or `&`, they decay to `char *`.

### `src/version.c` — Version Bump

Change `VERSION_STAGE` from `"13500000"` to `"13600000"`.

## Test Plan

**New test files in `test/valid/expressions/`:**

1. `test_sizeof_ptr_add_int__8.c` — `sizeof(p + 1)` → 8
2. `test_sizeof_local_arr_add_int__8.c` — `sizeof(arr + 0)` → 8 (local array)
3. `test_sizeof_global_arr_add_int__8.c` — `sizeof(arr + 0)` → 8 (global array)
4. `test_sizeof_char_arr_add_int__8.c` — `sizeof(buf + 3)` → 8 (char array)
5. `test_sizeof_double_ptr_add_int__8.c` — `sizeof(p + 2)` → 8 (double pointer)
6. `test_sizeof_ptr_sub_int__8.c` — `sizeof(p - 1)` → 8
7. `test_sizeof_ptr_sub_ptr__8.c` — `sizeof(p - arr)` → 8 (pointer difference)
8. `test_sizeof_string_lit_add_int__8.c` — `sizeof("hello" + 0)` → 8

**Regression tests:**

9. `test_sizeof_arr_no_decay__20.c` — `sizeof(arr)` → 20 (ensure direct array operand does not decay)
10. `test_sizeof_int_add_int__4.c` — `sizeof(x + y)` → 4 (ensure integer arithmetic unchanged)

## Implementation Order

1. Fix `src/codegen.c` — `AST_BINARY_OP` case with pointer/array guard
2. Add `AST_STRING_LITERAL` case to `src/codegen.c`
3. Bump version in `src/version.c`
4. Create 10 test files in `test/valid/expressions/`
5. Run full test suite: `./build.sh`
6. Self-host cycle: `./build.sh --mode=self-host`
7. Update checklist: mark "Lvalue conversion rules for all expression contexts" as complete
8. Commit with `Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>`

## Notes

- The fix is entirely contained within the `sizeof_type_of_expr` function; no other codegen paths or data structures are affected.
- Runtime behavior of pointer arithmetic is already correct; only compile-time type-sizing for `sizeof` was wrong.
- On LP64, both `ptrdiff_t` and pointer types are 8 bytes, so no distinction is needed; both branches return `TYPE_POINTER`.
- The pointer guard check is harmless if evaluated for invalid pointer operations (e.g., `ptr * ptr`); such invalid expressions would be caught by the parser or an existing compiler.
