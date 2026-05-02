# Stage-16-04 Kickoff

## Spec Summary

Stage 16-04 adds the three binary bitwise operators `&` (bitwise AND),
`^` (bitwise XOR), and `|` (bitwise OR). All three are integer-only
(`char`, `short`, `int`, `long`) — pointer and array operands are
rejected. Both operands undergo the usual integer promotion (char/short
→ int; mixed int/long → long). Three new grammar levels slot between
`<equality_expression>` and `<logical_and_expression>`, with precedence
in increasing tightness `|` < `^` < `&`. So `1 & 3 == 3` parses as
`1 & (3 == 3) → 1 & 1 → 1`, `1 | 2 & 4` parses as `1 | (2 & 4) → 1`,
and `1 ^ 3 & 1` parses as `1 ^ (3 & 1) → 0`. All three are
left-associative.

## What changes from stage 16-03

1. **Tokenizer** — two new tokens `TOKEN_CARET` (`^`) and `TOKEN_PIPE`
   (`|`). `|` was previously matched only as part of `||`; bare `|`
   now emits `TOKEN_PIPE`. `^` had no prior lexer rule and produced
   `TOKEN_UNKNOWN`. The existing `&` already lexes to
   `TOKEN_AMPERSAND` (the same token used for unary address-of), so
   the parser is what disambiguates binary `&` from unary `&`.
2. **Grammar** — insert three new precedence levels:
   ```
   <logical_and_expression> ::= <bitwise_or_expression>  { "&&" <bitwise_or_expression> }
   <bitwise_or_expression>  ::= <bitwise_xor_expression> { "|"  <bitwise_xor_expression> }
   <bitwise_xor_expression> ::= <bitwise_and_expression> { "^"  <bitwise_and_expression> }
   <bitwise_and_expression> ::= <equality_expression>    { "&"  <equality_expression> }
   ```
3. **Parser** — three new functions: `parse_bitwise_and` (above
   `parse_equality`), `parse_bitwise_xor` (above `parse_bitwise_and`),
   `parse_bitwise_or` (above `parse_bitwise_xor`). `parse_logical_and`
   now consumes `parse_bitwise_or`. Each produces `AST_BINARY_OP`
   with `value` "&" / "^" / "|"; left-associative.
4. **AST** — no new node types or fields.
5. **Codegen** — `&` / `^` / `|` use `and` / `xor` / `or`. Operand
   evaluation reuses the existing arithmetic-binary-op pattern:
   evaluate left → push → evaluate right → pop left into rcx → emit
   `and|xor|or eax, ecx` (32-bit common) or `and|xor|or rax, rcx`
   (64-bit common). Sign-extension via `movsxd` is applied to int
   sides when the common type is `long`, matching the existing
   `+`/`-`/`*`/`/`/`%` widening. Pointer or array operands on either
   side are rejected with the existing
   `error: operator '<op>' not supported on pointer operands`
   diagnostic.
6. **Pretty printer** — `operator_name` learns `&` → `BITAND`,
   `^` → `BITXOR`, `|` → `BITOR` so AST dumps stay readable.
7. **Tests** — basic, variables, precedence (`& == ==`, `| & &`,
   `^ & &`), left-associativity, mixed with logical-and, all-three
   chain, char/long promotion, plus invalid pointer/array on either
   side and missing-operand parse errors.

## Spec Issues Called Out

1. **Mixed logic operators example has a typo `reutrn`** — written as
   `reutrn 0 | 2 && 3;`. Treating as `return 0 | 2 && 3;`; the
   expected `1` is consistent with C precedence
   (`(0 | 2) && 3` → `2 && 3` → `1`).
2. **First INVALID example contains the literal `^&a`**:
   ```C
   int *p = ^&a;
   return p & 1;  //INVALID
   ```
   `^&a` is not a valid C expression. The intent is to reject
   `p & 1` because `p` is a pointer; the prefix `^` is unrelated to
   the property under test. Implemented as
   `int *p = &a; return p & 1;`, which exercises the spec's
   pointer-operand rejection cleanly.
3. **Second INVALID example uses `A | 1`** — `A` decays to a pointer
   and the operator rejects pointer operands. The spec test is taken
   verbatim and matches the existing decay-to-pointer rejection
   pattern from stage 16-03 shift tests.
4. **No explicit *result type* statement** — the spec describes 32-bit
   vs 64-bit codegen but does not name a result-type rule. Adopting
   the standard C "common arithmetic type" rule that the rest of
   the binary arithmetic operators already follow: char/short/int →
   int; long stays long; one side long → long. This is what the
   spec's codegen examples imply.
5. **Bare `|` previously fell through to `TOKEN_UNKNOWN`**. The
   existing lexer matches `||` but not bare `|`. With this stage
   `|` produces `TOKEN_PIPE`. No earlier code depended on the
   `TOKEN_UNKNOWN` behavior.

## Planned Changes

### Tokenizer
- `include/token.h` — add `TOKEN_CARET`, `TOKEN_PIPE`.
- `src/lexer.c` — emit `TOKEN_CARET` on `^`; emit `TOKEN_PIPE` on
  `|` when the next char is not `|`.
- `src/compiler.c` — extend `token_type_name`.

### Parser
- `src/parser.c` — new `parse_bitwise_and`, `parse_bitwise_xor`,
  `parse_bitwise_or`. `parse_logical_and` calls `parse_bitwise_or`.

### AST
- `src/ast_pretty_printer.c` — `operator_name` learns the three
  new operator strings.
- No new node types.

### Code Generator
- `src/codegen.c`:
  - Extend `expr_result_type` AST_BINARY_OP arm so `&` / `^` / `|`
    yield `common_arith_kind(left, right)`.
  - Extend `codegen_expression` AST_BINARY_OP arm with a new branch
    for `&` / `^` / `|`. Reject pointer/array operands. Reuse the
    `is_arith` widening shape; emit `and` / `xor` / `or`.

### Tests

Valid (`test/valid/`):
- `test_bitand_basic__2.c` — `return 6 & 3;`
- `test_bitor_basic__7.c` — `return 6 | 3;`
- `test_bitxor_basic__5.c` — `return 6 ^ 3;`
- `test_bitand_var_int__2.c` — `int a = 6; int b = 3; return a & b;`
- `test_bitor_precedence_and__1.c` — `return 1 | 2 & 4;`
- `test_bitxor_precedence_and__0.c` — `return 1 ^ 3 & 1;`
- `test_bitand_precedence_equality__1.c` — `return 1 & 3 == 3;`
- `test_bitand_left_associative__1.c` — `return 7 & 3 & 1;`
- `test_bitor_logical_and_mixed__1.c` — `return 0 | 2 && 3;`
- `test_bitor_xor_and_chain__7.c` — `return 1 | 2 ^ 4 & 7;`
- `test_bitand_char_promotion__2.c` — `char a = 6; char b = 3; return a & b;`
- `test_bitor_long_via_call__7.c` — long-operand `|` via callee.
- `test_bitand_long_via_call__2.c` — long-operand `&` via callee.

Invalid (`test/invalid/`):
- `test_invalid_83__not_supported_on_pointer_operands.c` — `p & 1`.
- `test_invalid_84__not_supported_on_pointer_operands.c` — `p | 1`.
- `test_invalid_85__not_supported_on_pointer_operands.c` — `p ^ 1`.
- `test_invalid_86__not_supported_on_pointer_operands.c` — `A & 1`.
- `test_invalid_87__not_supported_on_pointer_operands.c` — `A | 1`.
- `test_invalid_88__not_supported_on_pointer_operands.c` — `A ^ 1`.
- `test_invalid_89__expected_expression.c` — `return 1 &;`
- `test_invalid_90__expected_expression.c` — `return 1 |;`
- `test_invalid_91__expected_expression.c` — `return 1 ^;`
- `test_invalid_92__expected_expression.c` — `return ^ 1;`
- `test_invalid_93__expected_expression.c` — `return | 1;`

Print-tokens (`test/print_tokens/`):
- `test_token_caret.c` / `.expected` — `^` → `TOKEN_CARET`.
- `test_token_pipe.c` / `.expected` — `|` → `TOKEN_PIPE`.

Print-asm (`test/print_asm/`):
- `test_stage_16_04_bitand_int.c` — `return 6 & 3;` locks in
  `and eax, ecx`.
- `test_stage_16_04_bitor_int.c` — `return 6 | 3;` locks in
  `or eax, ecx`.
- `test_stage_16_04_bitxor_int.c` — `return 6 ^ 3;` locks in
  `xor eax, ecx`.
- `test_stage_16_04_bitand_long.c` — long-operand and via callee
  locks in `and rax, rcx`.

### Documentation
- `docs/grammar.md` — insert the three bitwise rules; rewrite
  `<logical_and_expression>` to consume `<bitwise_or_expression>`.
- `README.md` — bump "Through stage 16-03" to "Through stage 16-04";
  add bitwise-binary bullet; refresh test totals.

### Commit
Single `stage-16-04` commit covering tokenizer / parser / codegen /
pretty-printer / tests / docs after the full suite passes. Trailer
must include `Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>`.

## Test Plan

```
cmake --build build
./test/run_all_tests.sh
```

Expectation: prior 505 tests still pass; the 13 new valid + 11 new
invalid + 2 new print-tokens + 4 new print-asm tests pass; total
535 with no regressions.
