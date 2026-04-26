# stage-12-06 Transcript: Null Pointer Constant and Pointer Equality

## Summary

Stage 12-06 introduces the C null pointer constant: the integer literal
`0` may be used wherever a pointer is expected in three contexts
defined by the spec — declaration initializers for pointer locals,
`return` expressions from a pointer-returning function, and operands
of `==` / `!=` against a pointer. The change is codegen-only; grammar,
parser, AST, lexer, and pretty-printer remain unchanged.

The stage also implements pointer equality and inequality with strict
type rules. Two pointer operands must agree on their full type chain;
a pointer may be compared against the null pointer constant on either
side; a pointer compared with a non-zero integer is rejected;
relational comparisons (`<`, `>`, `<=`, `>=`) and arithmetic on
pointer operands remain out of scope. Comparisons of pointer operands
use a 64-bit `cmp` and skip the integer `movsxd` widening; the result
remains a 32-bit `int`.

The literal `0` is detected at the use site by inspecting the AST
node (`AST_INT_LITERAL` with value `"0"`); the lexer drops any
`L`/`l` suffix from the value text, so this also matches `0L`,
consistent with C99's definition of a null pointer constant.

## Plan

1. Pause to flag spec issues: the typo `!-`, "interger", a missing
   close paren, and the existing test `test_invalid_27` that the new
   spec contradicts.
2. Implementation order: codegen helper, `expr_result_type` updates,
   declaration init relaxation, return statement relaxation, binary-op
   pointer comparison, then tests, then docs / transcript / commit.
3. No grammar / parser / AST / pretty-printer changes (spec says
   "Grammar Changes: NONE").

## Changes Made

### Step 1: Code Generator

- Added `is_null_pointer_constant(ASTNode*)` returning true iff the
  node is `AST_INT_LITERAL` with `value == "0"`.
- Updated `expr_result_type` to return `TYPE_POINTER` for
  `AST_VAR_REF` of a pointer local and for `AST_ADDR_OF`, so the
  binary-op pre-walk can detect pointer comparisons.
- `AST_DECLARATION`: when the LHS is a pointer and the initializer is
  the null pointer constant, bypass the pointer/non-pointer mismatch
  error and store via the 8-byte path (`mov eax, 0` zero-extends to
  `rax`, so the full slot receives the null address).
- `AST_RETURN_STATEMENT`: when the function's declared return type is
  a pointer and the return expression is the null pointer constant,
  bypass the pointer/non-pointer mismatch error; no conversion is
  emitted.
- `AST_BINARY_OP`: in the type pre-walk, if either operand is
  `TYPE_POINTER`, force the common type to `TYPE_LONG` (64-bit cmp)
  and skip integer `movsxd` widening on pointer operands. Operators
  other than `==` / `!=` on pointer operands are rejected. After both
  sides are evaluated, validate the operand combination — two
  pointers must have equal type chains (`pointer_types_equal`); one
  pointer plus integer requires the integer side to be the null
  pointer constant; otherwise an error is reported.

### Step 2: Tests

- New valid tests in `test/valid/`:
  - `test_null_pointer_init__0.c`
  - `test_null_pointer_return__1.c`
  - `test_pointer_eq_same__1.c`
  - `test_pointer_neq_diff__1.c`
  - `test_pointer_eq_null__1.c`
  - `test_pointer_neq_null__1.c`
  - `test_null_eq_pointer__1.c`
- New invalid tests in `test/invalid/`:
  - `test_invalid_29__assigning_non_pointer_to_pointer.c` (`int *p = 5;`)
  - `test_invalid_30__incompatible_pointer.c` (`int *` vs `char *` in
    `==`)
  - `test_invalid_31__comparing_pointer_with_non_zero_integer.c`
- Deleted `test/invalid/test_invalid_27__returning_non_pointer.c`
  (its `int *f() { return 0; }` is now valid per spec; the
  non-zero-return rejection is still covered by
  `test_invalid_24__returning_non_pointer.c`).

## Final Results

All 256 tests pass (209 valid + 30 invalid + 17 print-AST). No regressions.

## Session Flow

1. Read the stage 12-06 spec and supporting files.
2. Reviewed the current codegen, parser, and existing pointer tests.
3. Drafted a kickoff document flagging spec typos and the test
   conflict.
4. Implemented the codegen changes in small steps: helper,
   `expr_result_type`, declaration init, return statement, binary-op.
5. Built the compiler; ran valid, invalid, and print-AST suites to
   confirm no regressions.
6. Deleted the now-contradictory invalid test and added 7 valid + 3
   invalid tests.
7. Re-ran all suites; recorded final results.
8. Wrote the milestone summary and this transcript.

## Notes

- The lexer drops the `L`/`l` suffix from integer literal value
  strings, so `0L` is detected as the null pointer constant by the
  same `value == "0"` check.
- `expr_result_type` is intentionally not extended to handle
  `AST_DEREF` for pointer-of-pointer expressions; nested-pointer
  equality (e.g. `*pp == p`) is outside the spec's listed test
  surface for this stage.
- The grammar in `docs/grammar.md` was not changed; the spec
  explicitly states "Grammar Changes: NONE".
