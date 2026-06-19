# Stage 140 Kickoff: Pointer-Size Typedef Behavior

## Summary of the Spec

**Spec ID**: CC99-013 (Medium Priority)

**Issue**: Casts to typedef unsigned types (like `size_t`) don't preserve unsigned arithmetic semantics.

**Failing Behavior**: The expression `(size_t)0 - (size_t)1 > 0` returns 105 (a failure code) instead of passing. The arithmetic should wrap to `ULONG_MAX` under unsigned semantics (which is > 0), but the compiler incorrectly treats it as signed.

**Root Cause**: In `parse_cast` (src/parser.c ~line 2236), `AST_CAST` nodes store `cast->decl_type = cast_type->kind` but never set `cast->is_unsigned`. The full type information is only stored for pointer casts, not integer casts. As a result, `(size_t)0` creates a cast node with `decl_type=TYPE_LONG, is_unsigned=0`, losing the unsigned information from the `size_t` typedef.

**Expected Behavior**: The cast should preserve the signedness of its target type through the entire arithmetic pipeline. The reduced test program (which includes checks for `size_t`, `ptrdiff_t`, and `intptr_t` sizes and behavior) should return 10.

## Required Tokenizer Changes

**None**. The tokenizer already handles typedef names correctly.

## Required Parser Changes

**Location**: `src/parser.c`, in the `parse_cast` function (~line 2236)

**Change**: After `cast->decl_type = cast_type->kind`, add:
```c
cast->is_unsigned = !cast_type->is_signed;
```

This records the cast target type's signedness on the AST node, ensuring that unsigned typedef casts (like `size_t`) retain their unsigned property.

**Rationale**: The parser already has access to `cast_type->is_signed` from the resolved typedef. Storing this on the cast node makes it available to codegen and the UAC (Usual Arithmetic Conversions) path.

## Required AST Changes

**None**. The `AST_CAST` node already has an `is_unsigned` field; we just need to set it.

## Required Code Generation Changes

**None needed**. The existing `AST_CAST` handler in `codegen.c` (~line 3800) sets `node->result_type` but does not clear `node->is_unsigned`. This means the parser-set value will survive to when UAC reads it during binary operations like subtraction and comparison.

## Test Plan

1. **Integration test `test_std_pointer_size_typedefs`**: Run the full reduced source from the spec. This verifies:
   - `size_t` arithmetic with wrap-around: `(size_t)0 - (size_t)1 > 0`
   - `sizeof(size_t)` equals `sizeof(void *)`
   - `sizeof(ptrdiff_t)` equals `sizeof(void *)`
   - `sizeof(intptr_t)` equals `sizeof(void *)`
   - Pointer subtraction produces signed `ptrdiff_t` values
   - Pointer-to-`intptr_t`-to-pointer round-trip works
   - Expected return value: 10

2. **Verify all existing tests pass**: The change is localized to parser-level cast handling and should not break any of the 1979 existing tests.

3. **Edge cases to verify**:
   - Cast to other unsigned typedefs (e.g., `uint32_t`, `unsigned long`)
   - Mixed signed/unsigned arithmetic in expressions with casts
   - Constant folding with unsigned cast operands

## Implementation Plan

1. **Parser fix**: Add `cast->is_unsigned` assignment in `parse_cast`
2. **Create integration test**: Add `test_std_pointer_size_typedefs` with the spec's reduced source
3. **Run full test suite**: Verify no regressions
4. **Document results**: Create milestone summary

