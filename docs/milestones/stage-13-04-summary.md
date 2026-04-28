# Stage-13-04 Milestone: Pointer Arithmetic

## Completed
- Pointer arithmetic in additive expressions:
  - `T * + integer → T *`
  - `integer + T * → T *`
  - `T * - integer → T *`
- The integer side is sign-extended to 64 bits and scaled by
  `sizeof(*p)` (skipped when the element size is 1) before being
  added to / subtracted from the pointer.
- Invalid combinations are rejected with explicit messages:
  - `T * + T *` ("cannot add two pointers")
  - `T * - T *` ("pointer subtraction not supported")
  - `integer - T *` ("cannot subtract pointer from integer")
  - any other operator with a pointer operand (multiplication,
    division, relational) — existing rejection retained.
- `&` (address-of) now accepts an array-subscript operand in
  addition to a plain identifier, so `long *p = &a[2];` parses and
  produces a pointer to the element. Codegen reuses
  `emit_array_index_addr` and tags the result `pointer-to-element`.
- `expr_result_type` for `+`/`-` reports `TYPE_POINTER` when one
  operand is a pointer, so dereference and assignment paths
  consuming `p + n` see the correct type.
- Existing 32-bit / 64-bit integer arithmetic, comparisons, and
  null-pointer handling are unchanged.

## Files Changed
- `src/parser.c` — `&` operand may also be `AST_ARRAY_INDEX`.
- `src/codegen.c`
  - `expr_result_type(AST_BINARY_OP)` for `+`/`-`: returns
    `TYPE_POINTER` on a pointer operand.
  - `AST_BINARY_OP` codegen — pointer-operand classifier extended
    with `is_pointer_arith`; explicit error messages for the
    rejected combinations; new pointer-arith emit block scales the
    integer side by element size and emits 64-bit `add` / `sub`.
  - `AST_ADDR_OF` codegen — when the operand is `AST_ARRAY_INDEX`,
    use `emit_array_index_addr` and produce `pointer-to-element`.
- `test/valid/` — 4 new tests:
  - `test_ptr_arith_int_plus_one__42.c`
  - `test_ptr_arith_long_plus_one__42.c`
  - `test_ptr_arith_one_plus_char__9.c`
  - `test_ptr_arith_long_minus_one__17.c`
- `test/invalid/` — 2 new tests:
  - `test_invalid_38__cannot_add_two_pointers.c`
  - `test_invalid_39__cannot_subtract_pointer_from_integer.c`

## Test Results
- Valid: 225 / 225 pass.
- Invalid: 38 / 38 pass.
- Print-AST: 19 / 19 pass.
- No regressions.
