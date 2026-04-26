# Stage-12-06 Milestone: Null Pointer Constant and Pointer Equality

## Completed
- The integer literal `0` is recognized as the C null pointer constant
  in three contexts: declaration initializer for a pointer local
  (`int *p = 0;`), `return` from a pointer-returning function
  (`int *f() { return 0; }`), and operands of `==` / `!=` against a
  pointer (`p == 0`, `0 != p`).
- Pointer equality and inequality (`==`, `!=`) are implemented with
  strict type rules: two pointers must have equal type chains; a
  pointer may be compared to the null pointer constant; a pointer
  compared with any non-zero integer is rejected; relational pointer
  comparisons remain out of scope.
- Comparisons against pointer operands use a 64-bit `cmp` and skip the
  integer `movsxd` widening (pointer values already live in the full
  `rax`). The result remains a 32-bit `int` (0 or 1), consistent with
  existing comparison behavior.

## Files Changed
- `src/codegen.c` —
  - New helper `is_null_pointer_constant`.
  - `expr_result_type`: pointer var-refs and `AST_ADDR_OF` now report
    `TYPE_POINTER` so the binary-op pre-walk can recognize pointer
    comparisons.
  - `AST_DECLARATION`: pointer-LHS / non-pointer-RHS rejection is
    bypassed when the initializer is the null pointer constant.
  - `AST_RETURN_STATEMENT`: pointer-return / non-pointer-expr
    rejection is bypassed when the expression is the null pointer
    constant.
  - `AST_BINARY_OP`: when either operand is a pointer, `==` / `!=`
    use the 64-bit `cmp` path; non-equality operators on pointers are
    rejected; after evaluation the operand combination is validated
    (chain-equal pointers, or one pointer plus null pointer constant).
- `test/valid/` — 7 new tests (null init, null return, pointer
  equality, pointer inequality, pointer-vs-null both directions).
- `test/invalid/` — 3 new tests (non-zero integer initializing pointer,
  mismatched pointer types in comparison, pointer compared with
  non-zero integer); 1 obsolete test deleted (`test_invalid_27`, which
  previously expected `int *f() { return 0; }` to be rejected — now
  valid per spec).

## Test Results
- Valid: 209 / 209 pass.
- Invalid: 30 / 30 pass.
- Print-AST: 17 / 17 pass.
- No regressions.
