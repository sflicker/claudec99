# Milestone Summary

## Stage-16-04: Bitwise binary operators (`&`, `^`, `|`) — Complete

stage-16-04 ships the binary bitwise operators `&` (AND), `^` (XOR),
and `|` (OR) on integer operands.

- Tokenizer: introduced `TOKEN_CARET` (`^`) and `TOKEN_PIPE` (`|`).
  Bare `|` previously fell through to `TOKEN_UNKNOWN`; `||` still
  matches first. `&` continues to use `TOKEN_AMPERSAND` for both the
  unary address-of and the binary bitwise AND — disambiguation is
  the parser's job. The `--print-tokens` formatter learned both new
  names.
- Grammar/Parser: inserted three new precedence levels between
  `<equality_expression>` and `<logical_and_expression>` —
  `<bitwise_and_expression>` (tightest), `<bitwise_xor_expression>`,
  `<bitwise_or_expression>`. `<logical_and_expression>` now consumes
  `<bitwise_or_expression>`. The new parsers (`parse_bitwise_and`,
  `parse_bitwise_xor`, `parse_bitwise_or`) are left-associative and
  produce `AST_BINARY_OP` with `value` "&" / "^" / "|".
- AST/Semantics: no new node types. The pretty printer's
  `operator_name` learned `&` → `BITAND`, `^` → `BITXOR`, `|` →
  `BITOR` for symmetry with `&&` / `||` (`AND` / `OR`).
- Codegen: `expr_result_type` reports `common_arith_kind(left, right)`
  for the three new operators (char/short/int → int; either side
  long → long). Emission evaluates left → push, right → pop into
  rcx, optionally `movsxd` widening to long, then `and|xor|or eax,
  ecx` (32-bit) or `and|xor|or rax, rcx` (64-bit). Pointer or array
  operands on either side are rejected at codegen with
  `error: operator '<op>' not supported on pointer operands`.
- Spec issues called out and resolved during kickoff: the typo
  `reutrn` in the mixed-logic example (treated as `return`); the
  literal `^&a` in the first INVALID example (rewritten to
  `&a`, which preserves the spec's pointer-operand rejection
  intent); no explicit result-type rule (adopted the standard
  common-arithmetic-type rule already used by `+`/`-`/`*`/`/`/`%`).
- Tests added (13 valid, 11 invalid, 2 print-tokens, 4 print-asm):
  basic operands, integer variables, precedence vs `==`, vs `&` /
  `^` / `|` themselves, vs `&&`, all-three chain, char and long
  promotion via callee, plus pointer/array rejection on each
  operator and missing-operand parse errors. New print-asm tests
  lock in `and eax, ecx`, `or eax, ecx`, `xor eax, ecx`, and
  `and rax, rcx`.
- Documentation: `docs/grammar.md` adds the three new bitwise
  productions and rewrites `<logical_and_expression>` to consume
  `<bitwise_or_expression>`; `README.md` bumped to "Through stage
  16-04" with a new bitwise-binary-operators bullet and refreshed
  test totals.
- Full suite: 535/535 pass (321 valid + 91 invalid + 24 print-AST +
  80 print-tokens + 19 print-asm). No regressions from prior stages.
