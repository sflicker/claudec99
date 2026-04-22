# Stage-11-04 Milestone: Types for Equality, Logical, and Conditional Expressions

Extended the stage 11-03 promotion / common-type machinery from
arithmetic operators to comparison, equality, logical, and control-flow
condition expressions so they behave correctly across `char`, `short`,
`int`, and `long`.

## What was completed
- Comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`) now select a
  common integer type after promotion. When the common type is `long`,
  int-sized operands are widened with `movsxd rax, eax` before the
  comparison and a 64-bit `cmp rcx, rax` is emitted; otherwise the
  32-bit `cmp ecx, eax` path is retained. Result type is `int`.
- Unary `!` tests the full width of its operand (`cmp rax, 0` for
  `long`, `cmp eax, 0` otherwise); result is `int`.
- Short-circuit `&&` and `||` test each operand's truthiness at the
  operand's own result width; results remain normalized `0` / `1` of
  type `int`.
- Control-flow condition tests in `if`, `while`, `do-while`, and `for`
  use a new `emit_cond_cmp_zero` helper that branches on the full
  width of the condition expression when it has a `long` result type.
- Fixed a typo in the pre-committed stage-11-04 test
  `test_long_if_cond__1.c` (function was `test` instead of `main`).
- Added new valid tests: `long == long`, `long != int`, `int >= long`,
  `!` on zero / nonzero long, `long && short`, `long || int` with
  both zero, `long`-driven `while`, `long`-driven `for`, and
  `long`-driven `do-while`.

## Scope limits (per spec)
Unsigned types, casts, bitwise / shift operators, ternary `?:`, typed
parameter passing, typed return conversions, pointer comparisons, and
floating-point are out of scope. No grammar changes.

## Test results
All 189 tests pass (163 valid + 14 invalid + 12 print_ast). No
regressions.
