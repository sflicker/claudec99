# ClaudeC99 Stage 136 — sizeof of Pointer-Arithmetic Expressions

## Overview

Stage 136 fixes a bug in `sizeof_type_of_expr` (in `src/codegen.c`) where
`sizeof` applied to a pointer-arithmetic expression returns the element size
(4) instead of the pointer/ptrdiff_t size (8).

**Affected cases** — all return 4, all should return 8 on LP64:

```
sizeof(int_ptr + 1)    // pointer + integer → pointer (size 8)
sizeof(arr + 0)        // array decays to pointer + integer → pointer (size 8)
sizeof(double_ptr - 2) // pointer - integer → pointer (size 8)
sizeof(ptr1 - ptr2)    // pointer - pointer → ptrdiff_t (size 8)
sizeof("hi" + 0)       // string literal decays to pointer → pointer (size 8)
```

All other `sizeof` contexts already work correctly (direct var refs, subscript,
deref, address-of, cast, function call). The bug is limited to the case where
`sizeof` is applied to a *binary expression* whose result is a pointer type.

This stage addresses the checklist item:
> `[ ] Lvalue conversion rules for all expression contexts`

## Root Cause

`sizeof_type_of_expr` (line ~1827, `src/codegen.c`) is called for the
fallback path of `AST_SIZEOF_EXPR`. Its `AST_BINARY_OP` case (line ~1875)
computes the result kind by calling `promote_kind` on each operand and then
`common_arith_kind`. But `promote_kind(TYPE_POINTER)` and
`promote_kind(TYPE_ARRAY)` both fall through to the function's default,
returning `TYPE_INT` — so pointer arithmetic is mis-typed as integer
arithmetic and produces size 4.

Additionally, `sizeof_type_of_expr` has no `AST_STRING_LITERAL` case; string
literals fall to `default: return TYPE_INT`, so `sizeof("hi" + 0)` also
returns 4 instead of 8.

## Required Changes

### `src/codegen.c` — `sizeof_type_of_expr`

**Change 1 — `AST_BINARY_OP` case**

Before calling `promote_kind`/`common_arith_kind`, check whether either
operand resolves to a pointer or array type. If so, return `TYPE_POINTER` for
`+` and `-` (both `ptr ± integer → pointer` and `ptr - ptr → ptrdiff_t` have
size 8 on LP64; no distinction is needed here).

Current flow (simplified):
```c
TypeKind lt = sizeof_type_of_expr(cg, node->children[0]);
TypeKind rt = sizeof_type_of_expr(cg, node->children[1]);
...
return common_arith_kind(promote_kind(lt), promote_kind(rt));
```

Required addition — insert before the `promote_kind` path, inside the
`+`/`-`/`*`/`/`/`%`/`&`/`^`/`|` branch:
```c
/* Pointer arithmetic: (array|pointer) ± integer → pointer;
 * pointer - pointer → ptrdiff_t; both are size 8 on LP64. */
if (lt == TYPE_POINTER || lt == TYPE_ARRAY ||
    rt == TYPE_POINTER || rt == TYPE_ARRAY) {
    return TYPE_POINTER;
}
```

This check must come after the FP check (so FP expressions still go through
`fp_common_arith_kind`) but before the integer `promote_kind` path.

**Change 2 — `AST_STRING_LITERAL` case**

String literals have type `char[N]`; in any expression context other than
direct `sizeof` or `&`, they decay to `char *`. Add a case so that when a
string literal appears as a sub-expression of a binary op, it is treated as a
pointer:
```c
case AST_STRING_LITERAL:
    return TYPE_POINTER;
```

No other changes to `sizeof_type_of_expr` are needed.

## Test Plan

Eight new test files in `test/valid/expressions/`:

1. **`test_sizeof_ptr_add_int__8.c`**
   ```c
   int main(void) { int x; int *p = &x; return sizeof(p + 1); }
   ```
   Exit code: `8`

2. **`test_sizeof_local_arr_add_int__8.c`**
   ```c
   int main(void) { int arr[5]; return sizeof(arr + 0); }
   ```
   Exit code: `8`

3. **`test_sizeof_global_arr_add_int__8.c`**
   ```c
   int arr[5];
   int main(void) { return sizeof(arr + 0); }
   ```
   Exit code: `8`

4. **`test_sizeof_char_arr_add_int__8.c`**
   ```c
   int main(void) { char buf[10]; return sizeof(buf + 3); }
   ```
   Exit code: `8`

5. **`test_sizeof_double_ptr_add_int__8.c`**
   ```c
   int main(void) { double d; double *p = &d; return sizeof(p + 2); }
   ```
   Exit code: `8`

6. **`test_sizeof_ptr_sub_int__8.c`**
   ```c
   int main(void) { int x; int *p = &x; return sizeof(p - 1); }
   ```
   Exit code: `8`

7. **`test_sizeof_ptr_sub_ptr__8.c`**
   ```c
   int main(void) { int arr[4]; int *p = arr + 2; return sizeof(p - arr); }
   ```
   Exit code: `8`

8. **`test_sizeof_string_lit_add_int__8.c`**
   ```c
   int main(void) { return sizeof("hello" + 0); }
   ```
   Exit code: `8`

Two regression tests (already-passing behavior must be preserved):

9. **`test_sizeof_arr_no_decay__20.c`** (regression guard)
   ```c
   int main(void) { int arr[5]; return sizeof(arr); }
   ```
   Exit code: `20` — direct array operand of sizeof must NOT decay.

10. **`test_sizeof_int_add_int__4.c`** (regression guard)
    ```c
    int main(void) { int x = 1, y = 2; return sizeof(x + y); }
    ```
    Exit code: `4` — integer arithmetic result unchanged.

## Version Bump

Change `VERSION_STAGE` in `src/version.c` from `"13500000"` to `"13600000"`.

## Implementation Order

1. `src/codegen.c` — add pointer/array guard in `sizeof_type_of_expr`'s
   `AST_BINARY_OP` case
2. `src/codegen.c` — add `AST_STRING_LITERAL` case to `sizeof_type_of_expr`
3. `src/version.c` — version bump to `13600000`
4. Add the 10 test files listed above
5. Run full test suite: `./build.sh`
6. Self-host cycle: `./build.sh --mode=self-host`
7. Update checklist: mark `[ ] Lvalue conversion rules for all expression
   contexts` as `[x]`
8. Commit with `Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>`

## Notes

- The fix is contained to `sizeof_type_of_expr`; no other codegen paths are
  affected. Runtime behavior of pointer arithmetic is already correct — only
  the compile-time type-sizing for sizeof was wrong.
- `ptr - ptr` and `ptr ± int` both return `TYPE_POINTER` (size 8). On LP64,
  `ptrdiff_t` is also 8 bytes, so no separate `TYPE_LONG` path is needed.
- The `AST_BINARY_OP` pointer guard must not fire for `*`, `/`, `%`, `&`, `^`,
  `|` with pointer operands (those are invalid C anyway), so returning
  `TYPE_POINTER` there is harmless.
