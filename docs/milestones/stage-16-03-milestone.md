# Milestone Summary

## Stage-16-03: Shift operators (`<<`, `>>`) — Complete

- Added the binary shift operators `<<` (left shift) and `>>`
  (arithmetic right shift) end-to-end.
- Tokenizer: introduced `TOKEN_LEFT_SHIFT` and `TOKEN_RIGHT_SHIFT`.
  `<<` and `>>` are matched before the single-character `<` / `>`
  (and the existing `<=` / `>=`) so `1 << 3` cannot be mis-tokenized
  as two `<` tokens. The `--print-tokens` formatter learned the new
  token names.
- Grammar: inserted `<shift_expression>` between
  `<relational_expression>` and `<additive_expression>`. Shift
  therefore binds less tightly than `+`/`-` and more tightly than
  `<`/`>`/`<=`/`>=`, matching standard C precedence.
- Parser: new `parse_shift` between `parse_relational` and
  `parse_additive`. `parse_relational` now consumes
  `<shift_expression>`. The new operators reuse `AST_BINARY_OP` with
  `value = "<<"` or `">>"`; no new AST shapes are required.
- Code generator: `expr_result_type` reports the **promoted left
  operand's** type for `<<` / `>>` — the right operand is a shift
  count and does not participate in common-arithmetic typing.
  Emission evaluates left into rax (push), right into rax (pop left
  into rcx), then `xchg rax, rcx` so left lands in rax and the
  count's low byte is addressable as `cl`. The instruction is
  `shl`/`sar` against `eax` for promoted-int operands and against
  `rax` for `long` operands.
- `<<` and `>>` reject pointer or array (decayed) operands on either
  side at codegen with
  `error: operator '<op>' not supported on pointer operands`,
  reusing the diagnostic shape from `%` and `~`.
- Spec issue called out and resolved during kickoff: the spec's long
  right-shift test used `>>>`, which is not a C operator. Treated as
  the typo it was; only `<<` and `>>` are added.
- Tests added (15 valid, 8 invalid, 2 print-tokens, 3 print-asm):
  - valid: `test_lshift_basic_1__8.c`, `test_lshift_basic_2__12.c`,
    `test_rshift_basic_1__4.c`, `test_rshift_basic_2__4.c`,
    `test_rshift_arith_negative__252.c` (sign-preserving `sar`),
    `test_lshift_var_int__8.c`, `test_rshift_var_int__4.c`,
    `test_shift_precedence_additive__24.c`
    (`1 + 2 << 3` → 24),
    `test_shift_precedence_relational__1.c`
    (`1 << 3 > 4` → 1),
    `test_shift_left_associative__4.c`
    (`32 >> 2 >> 1` → 4),
    `test_lshift_char_promotion__4.c`,
    `test_rshift_char_promotion__4.c`,
    `test_lshift_short_promotion__16.c`,
    `test_lshift_long_via_call__32.c`,
    `test_rshift_long_via_call__4.c`.
  - invalid: `test_invalid_75__not_supported_on_pointer_operands.c`
    (`p << 1`),
    `test_invalid_76__not_supported_on_pointer_operands.c`
    (`p >> 2`),
    `test_invalid_77__not_supported_on_pointer_operands.c`
    (`A << 1` via array decay),
    `test_invalid_78__not_supported_on_pointer_operands.c`
    (`A >> 1` via array decay),
    `test_invalid_79__expected_expression.c` (`return 1 <<;`),
    `test_invalid_80__expected_expression.c` (`return 1 >>;`),
    `test_invalid_81__expected_expression.c` (`return >> 1;`),
    `test_invalid_82__expected_expression.c` (`return << 1;`).
  - print-tokens: `test_token_left_shift.c`, `test_token_right_shift.c`.
  - print-asm: `test_stage_16_03_lshift_int.c` (32-bit `shl eax, cl`),
    `test_stage_16_03_rshift_int.c` (32-bit `sar eax, cl`),
    `test_stage_16_03_lshift_long.c` (64-bit `shl rax, cl`).
- Documentation: `docs/grammar.md` adds `<shift_expression>` and
  rewrites `<relational_expression>` to consume it; `README.md`
  bumped to "Through stage 16-03" with a new shift-operators bullet
  and refreshed test totals.
- Full suite: 505/505 pass (308 valid + 80 invalid + 24 print-AST +
  78 print-tokens + 15 print-asm). No regressions from prior stages.
