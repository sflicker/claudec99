# stage-13-04 Transcript: Pointer Arithmetic

## Summary

This stage adds pointer arithmetic to the additive operators.
A pointer plus or minus an integer produces a pointer; the
integer side is sign-extended to 64 bits and scaled by the
pointed-to type size before being added or subtracted. The
combinations `pointer + pointer`, `pointer - pointer`, and
`integer - pointer` are explicitly rejected with descriptive
errors; pointer operands continue to be rejected for `*`, `/`,
and the relational operators.

Address-of was extended to accept an array-subscript operand so
`&a[i]` produces a pointer to the i-th element of the array.
This was required by the spec's fourth test case
(`long *p = &a[2];`).

The deref and deref-LHS paths consume the pointer-arithmetic
result through their existing logic — once `p + n` reports
`TYPE_POINTER` and the right `full_type` chain, both `*(p + n)`
reads and `*(p + n) = rhs` writes work without further changes.

## Changes Made

### Step 1: Parser

- `parse_unary` `&` branch: accept `AST_ARRAY_INDEX` operand in
  addition to `AST_VAR_REF`.

### Step 2: Code Generator

- `expr_result_type(AST_BINARY_OP)` for `+`/`-`: when one operand
  is a pointer, return `TYPE_POINTER` instead of falling through
  to the integer common-type rule.
- `AST_BINARY_OP` codegen: extended the pointer-operand
  classifier. Added `is_pointer_arith`. Explicit errors for the
  rejected combinations:
  - `T* + T*` — "cannot add two pointers"
  - `T* - T*` — "pointer subtraction not supported"
  - `int - T*` — "cannot subtract pointer from integer"
- New pointer-arith emit block (after the `pop rcx`): scales the
  integer side by `sizeof(*p)` (skipped when 1), then emits a
  64-bit `add` (for `+` and `int + ptr`) or `sub`+`mov` (for
  `ptr - int`). The result keeps the pointer's `full_type`.
- `AST_ADDR_OF` codegen: when the operand is `AST_ARRAY_INDEX`,
  call `emit_array_index_addr` and tag the result
  `pointer-to-element`.

### Step 3: Tests

- `test/valid/test_ptr_arith_int_plus_one__42.c` — `int *p`,
  `*(p + 1) = 42`.
- `test/valid/test_ptr_arith_long_plus_one__42.c` — `long *p`,
  `*(p + 1) = 42`.
- `test/valid/test_ptr_arith_one_plus_char__9.c` — `char *p`,
  `*(1 + p) = 9`.
- `test/valid/test_ptr_arith_long_minus_one__17.c` —
  `long *p = &a[2]; *(p - 1) = 17;`.
- `test/invalid/test_invalid_38__cannot_add_two_pointers.c` —
  `return p + q;`.
- `test/invalid/test_invalid_39__cannot_subtract_pointer_from_integer.c` —
  `return 1 - p;`.

## Final Results

All suites pass: 225 valid (+4), 38 invalid (+2), 19 print-AST.
No regressions.

## Session Flow

1. Read the stage spec and surveyed parser/codegen for binary
   ops, addr-of, deref.
2. Wrote and saved the kickoff document.
3. Extended the parser to accept `&a[i]`.
4. Extended `AST_ADDR_OF` codegen to handle the array-index case.
5. Extended `expr_result_type` for `+`/`-` and the binary-op
   pointer-operand classifier; added the pointer-arith emit
   block.
6. Added 4 valid + 2 invalid tests.
7. Built and ran all suites — all green on first run.
8. Wrote milestone and transcript.

## Notes

- The spec heading reads "Stage 13.4" but the filename is
  `ClaudeC99-spec-stage-13-04-pointer-arithmetic.md`. The skill
  rules derive `STAGE_LABEL` from the filename.
- Pointer subtraction (`T * - T *`) is listed as out of scope by
  the spec; the implementation rejects it with a dedicated
  message rather than letting it fall through to a generic
  unsupported-operator error.
- Grammar is unchanged — `&` and `+`/`-` were already in the
  grammar.
