# Milestone Summary

## Stage-16-01: Remainder operator — Complete

- Added the binary `%` (remainder) operator end-to-end.
- Tokenizer: introduced `TOKEN_PERCENT` for the `%` character (no
  compound `%=` form). The `--print-tokens` formatter learned the
  new token name.
- Grammar: `<multiplicative_expression>` extended to admit `%`
  alongside `*` and `/`, sharing their left-associativity and
  precedence.
- Parser/AST: `parse_term` now also consumes `TOKEN_PERCENT`,
  producing the existing `AST_BINARY_OP` with `value = "%"` — no
  new AST shape required.
- Code generator: 32-bit operands emit
  `xchg eax, ecx; cdq; idiv ecx; mov eax, edx`; 64-bit operands
  emit `xchg rax, rcx; cqo; idiv rcx; mov rax, rdx`. The remainder
  is read out of `edx` / `rdx`. Pointer or array (decayed)
  operands are rejected by the existing
  `error: operator '%' not supported on pointer operands`
  diagnostic. Division-by-zero is not checked in this stage, per
  spec.
- Char/short operands are promoted to `int` through the existing
  `common_arith_kind` path; mixed `int` / `long` is widened to
  `long` exactly as for `*` and `/`.
- Tests added (10 valid, 5 invalid):
  - valid: `test_remainder_basic_zero__0.c`,
    `test_remainder_basic_one__1.c`,
    `test_remainder_basic_two__2.c`,
    `test_remainder_precedence_mul__1.c`,
    `test_remainder_precedence_div__2.c`,
    `test_remainder_left_assoc__0.c`,
    `test_remainder_variables__1.c`,
    `test_remainder_in_if__1.c`,
    `test_remainder_in_while__5.c`,
    `test_remainder_long__2.c`.
  - invalid: `test_invalid_66__expected_expression.c` (`return %;`),
    `test_invalid_67__expected_expression.c` (`return 1%;`),
    `test_invalid_68__expected_expression.c` (`return %1;`),
    `test_invalid_69__not_supported_on_pointer_operands.c`
    (`p % b`),
    `test_invalid_70__not_supported_on_pointer_operands.c`
    (`A % b`, via array decay).
- Documentation: `docs/grammar.md`
  `<multiplicative_expression>` updated to list `%`; `README.md`
  bumped to "Through stage 16-01" with a new
  arithmetic-operators bullet and refreshed test totals.
- Spec note: the spec's left-associativity test states
  `return 2 % 4 % 2; // expect 2`, but the correct value under
  C's left-associative `%` is `(2 % 4) % 2 = 0`. The test source
  is preserved verbatim; the expected exit code is `0`.
- Full suite: 458/458 pass (284 valid + 68 invalid + 24 print-AST
  + 74 print-tokens + 8 print-asm). No regressions from prior
  stages.
