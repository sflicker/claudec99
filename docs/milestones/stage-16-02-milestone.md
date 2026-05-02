# Milestone Summary

## Stage-16-02: Unary bitwise complement (`~`) — Complete

- Added the unary bitwise complement operator `~` end-to-end.
- Tokenizer: introduced `TOKEN_TILDE` for the `~` character (no
  compound `~=` form). The `--print-tokens` formatter learned the
  new token name.
- Grammar: `<unary_operator>` extended to include `~`, sharing the
  precedence and associativity of the other unary operators.
- Parser/AST: `parse_unary` now also consumes `TOKEN_TILDE`,
  producing the existing `AST_UNARY_OP` with `value = "~"` — no new
  AST shape required.
- Code generator: `expr_result_type` treats `~` like unary `+`/`-`
  (result is the promoted operand type). Emission is `not eax` for
  promoted-int operands and `not rax` for `long` operands. Char and
  short are sign-extended into `eax` by the existing
  `emit_load_local` so no extra promotion is needed.
- `~` rejects pointer or array (decayed) operands at codegen with
  `error: operator '~' not supported on pointer operands`, mirroring
  the pattern used by `%` in stage 16-01.
- Beyond the spec: `!` was tightened in this stage to also reject
  pointer/array operands with the same diagnostic shape
  (`error: operator '!' not supported on pointer operands`). This is
  a deliberate scope addition agreed during kickoff so `~` and `!`
  share a single integer-only rule.
- Tests added (9 valid, 4 invalid, 1 print-tokens, 2 print-asm):
  - valid: `test_complement_basic_neg3__253.c`,
    `test_complement_basic_neg1__255.c`,
    `test_complement_basic_zero__0.c`,
    `test_complement_nested__5.c`,
    `test_complement_var_int__253.c`,
    `test_complement_var_char__253.c`,
    `test_complement_var_short__253.c`,
    `test_complement_var_long__253.c`,
    `test_complement_precedence__1.c`.
  - invalid: `test_invalid_71__not_supported_on_pointer_operands.c`
    (`~p`), `test_invalid_72__not_supported_on_pointer_operands.c`
    (`~A` via array decay),
    `test_invalid_73__not_supported_on_pointer_operands.c` (`!p`),
    `test_invalid_74__not_supported_on_pointer_operands.c` (`!A`
    via array decay).
  - print-tokens: `test_token_tilde.c` — single `~` lexes as
    `TOKEN_TILDE`.
  - print-asm: `test_stage_16_02_complement_int.c` (32-bit
    `not eax`) and `test_stage_16_02_complement_long.c` (64-bit
    `not rax`) lock in the emitted instruction sequence.
- Documentation: `docs/grammar.md` `<unary_operator>` updated to
  list `~`; `README.md` bumped to "Through stage 16-02" with a new
  unary-operators bullet and refreshed test totals.
- Full suite: 477/477 pass (293 valid + 72 invalid + 24 print-AST +
  76 print-tokens + 12 print-asm). No regressions from prior stages.
