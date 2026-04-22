# Stage-11-04 Kickoff: Types for Equality, Logical, and Conditional Expressions

## Spec Summary
Stage 11-04 extends the stage 11-03 promotion / common-type rules from
arithmetic to non-arithmetic integer expressions. Relational and
equality operators (`==`, `!=`, `<`, `<=`, `>`, `>=`) must promote
both operands, select a common integer type (`long` if either side is
`long`, else `int`), and compare at that width. Logical operators
(`&&`, `||`, `!`) must treat operands as truth values using the full
width of the operand, and produce a normalized `0` or `1`. Control-flow
condition expressions in `if`, `while`, `do-while`, and `for` must
evaluate truthiness consistently across all supported integer widths.
All comparison and logical expressions produce an `int` result.

## What Must Change From Stage 11-03
- Binary comparison codegen must choose a common type (int or long) and
  emit `cmp rcx, rax` instead of `cmp ecx, eax` when the common type is
  `long`, sign-extending int-width operands with `movsxd rax, eax`
  first.
- Unary `!` must compare the full operand width (`cmp rax, 0` when the
  operand type is `long`, else `cmp eax, 0`).
- `&&` and `||` must compare each operand's full width when that
  operand's result type is `long`; their own result type remains `int`.
- `if`, `while`, `do-while`, and `for` condition checks must branch on
  the full width of the condition when the condition's result type is
  `long`.
- Result type tracking for comparison and logical expressions stays at
  `int` (already handled by stage 11-03 scaffolding).

## Out of Scope
Unsigned types, casts, bitwise / shift operators, ternary `?:`, typed
parameter passing or return-value conversions, pointer comparisons,
floating point, diagnostics for suspicious comparisons.

## Spec Notes / Ambiguity
- The "Type rules for this stage" section lists `long` `long` — the
  intended arrow is missing. Interpreted as `long -> long`; no
  implementation impact.
- Pre-committed test `test/valid/test_long_if_cond__1.c` uses
  `int test()` rather than `int main()`. With the `-e main` link step
  this produces a segfault (exit 139). Treated as a typo; renamed to
  `main` alongside the stage work so the test can exercise its intent.

## Planned Changes
- **Codegen**: promote/common-type path added for the six comparison
  operators (with 64-bit `cmp` when common is `long`); full-width
  truthiness tests for `!`, `&&`, `||`; size-aware condition tests in
  `if` / `while` / `do-while` / `for`.
- **Parser / AST / Tokenizer**: no changes.
- **Tests**: add valid tests for long-vs-long equality, long relational
  mixed with int, mixed-width `&&` / `||`, `!` on a long, and a `while`
  condition driven by a long counter. Fix the typo in
  `test_long_if_cond__1.c`.
- **Grammar**: none (spec states no grammar changes).
