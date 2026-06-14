# Stage 119 Kickoff: FP Compound Assignment on Struct Members

## Summary

This is a **codegen-only stage**. The task is to fix compound assignment (`+=`, `-=`, `*=`, `/=`) on `float` and `double` struct fields for global (file-scope) struct variables and pointers-to-struct.

After Stage 117 (which fixed FP struct member reads), compound assignments work correctly when:
- The struct is local
- One operand is a known-FP literal
- Base expressions are simple pointer dereferences or array subscripts

But when **both operands are fields of a global struct** (e.g., `g.x += g.y`), the FP arithmetic path is silently skipped and integer instructions are emitted on floating-point bit patterns, producing garbage results.

The root cause: five locations in `src/codegen.c` where `expr_result_type()` and `sizeof_type_of_expr()` look up struct/field information using `codegen_find_var()` for local variables, but fail to fall back to `codegen_find_global()` when the variable is file-scope. This causes the functions to return `TYPE_INT` instead of the correct `TYPE_DOUBLE`, preventing the FP code path from activating.

---

## Required Tokenizer Changes

**None.** No new tokens needed.

---

## Required Parser Changes

**None.** No AST structure changes needed. The compound assignment desugaring (`a op= b` → `a = (a op b)`) already works correctly at parse time.

---

## Required AST Changes

**None.** The AST representation is already correct.

---

## Required Code Generation Changes

**Five fixes in `src/codegen.c`:**

### Bug 1: `expr_result_type()` — `AST_MEMBER_ACCESS` with `AST_VAR_REF` base (line ~2097)

When `codegen_find_var()` returns `NULL` for a global struct variable, fall back to `codegen_find_global()`.

**Pattern:** `g.x` where `g` is a file-scope struct.

### Bug 2: `expr_result_type()` — `AST_MEMBER_ACCESS` with `AST_DEREF` base (line ~2118)

When `codegen_find_var()` returns `NULL` for a global pointer, fall back to `codegen_find_global()`.

**Pattern:** `(*gp).x` where `gp` is a file-scope pointer-to-struct.

### Bug 3: `expr_result_type()` — `AST_ARROW_ACCESS` (line ~2180)

When `codegen_find_var()` returns `NULL` for a global pointer, fall back to `codegen_find_global()`.

**Pattern:** `gp->x` where `gp` is a file-scope pointer-to-struct.

### Bug 4: `sizeof_type_of_expr()` — `AST_MEMBER_ACCESS` with `AST_VAR_REF` base (line ~1856)

Same fix: add global fallback after local lookup fails.

### Bug 5: `sizeof_type_of_expr()` — `AST_ARROW_ACCESS` (line ~1910)

Same fix: add global fallback after local lookup fails.

Each fix follows the same pattern:
1. Extract struct/pointer type into a temporary variable
2. Try local lookup via `codegen_find_var()`
3. If that fails, try global lookup via `codegen_find_global()`
4. Continue with field resolution using the resolved struct/pointer type

**Related changes:**
- Bump `src/version.c`: `VERSION_STAGE` → `"01190000"`

---

## Test Plan

### New tests to create in `test/valid/structs/`

1. **`test_global_struct_fp_add_assign__7.c`** — Global struct, both fields FP, `+=` operator. Expected exit: 7

2. **`test_global_struct_fp_sub_assign__2.c`** — Global struct, both fields FP, `-=` operator. Expected exit: 2

3. **`test_global_struct_fp_mul_assign__6.c`** — Global struct, both fields FP, `*=` operator. Expected exit: 6

4. **`test_global_ptr_struct_fp_add_assign__9.c`** — Global pointer-to-struct, arrow operator, `+=`. Expected exit: 9

5. **`test_local_struct_fp_add_assign__5.c`** — Local struct (regression test). Expected exit: 5

6. **`test_mixed_struct_fp_assign__10.c`** — Mixed local and global struct fields. Expected exit: 10

7. **`test_global_struct_fp_accumulate__15.c`** — Global struct in loop accumulation pattern. Expected exit: 15

### Testing sequence

1. Build with CMake
2. Run existing test suite to ensure no regressions
3. Verify motivating case: `struct Vec2 g; g.x=3.0; g.y=4.0; g.x+=g.y; return (int)g.x;` exits with 7
4. Add the seven new tests
5. Run full test suite
6. Self-host bootstrap and verify all three passes produce identical output

---

## Implementation Notes

- The spec provides exact before/after code for each of the five fixes
- No structural changes to AST or parser; this is pure codegen cleanup
- Bootstrap cycle should be unaffected (compiler source doesn't use global FP struct fields in arithmetic)
- Tests focus on the previously-broken case: global struct FP member compound assignment
