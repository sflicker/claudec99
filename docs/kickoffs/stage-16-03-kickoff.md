# Stage-16-03 Kickoff

## Spec Summary

Stage 16-03 adds the binary shift operators `<<` (left shift) and `>>`
(arithmetic right shift). Both operands must be integer types
(`char`, `short`, `int`, `long`); pointer and array operands are
invalid. Both operands undergo the usual integer promotion. The
result type matches the **promoted type of the left operand** — the
right operand is purely a shift count and does not participate in
common-arithmetic typing. The right shift is arithmetic (`sar`),
preserving sign. A new grammar level `<shift_expression>` slots
between `<relational_expression>` (lower precedence) and
`<additive_expression>` (higher precedence): shift binds less tightly
than `+`/`-` and more tightly than `<`/`>`/`<=`/`>=`. Bad shift
counts (negative, ≥ width) are explicitly out of scope.

## What changes from stage 16-02

1. **Tokenizer** — two new tokens `TOKEN_LEFT_SHIFT` (`<<`) and
   `TOKEN_RIGHT_SHIFT` (`>>`). Compound forms match before the
   single-character `<` / `>` (and alongside the existing `<=` /
   `>=`).
2. **Grammar** — insert `<shift_expression>` between
   `<relational_expression>` and `<additive_expression>`.
   `<relational_expression>` is rewritten to consume
   `<shift_expression>`.
3. **Parser** — new `parse_shift` between `parse_relational` and
   `parse_additive`. Produces `AST_BINARY_OP` with `value = "<<"` or
   `">>"`. Left-associative.
4. **AST** — no new node types or fields.
5. **Codegen** — `<<` / `>>` use `shl` / `sar`, keyed on the **left**
   operand's promoted type:
   - left promoted to `int`: `shl eax, cl` / `sar eax, cl`, result
     type `TYPE_INT`.
   - left promoted to `long`: `shl rax, cl` / `sar rax, cl`, result
     type `TYPE_LONG`.
   The right operand is evaluated, promoted to integer, and only its
   low byte (`cl`) is consumed. Pointer/array operands on either side
   are rejected with the existing
   `error: operator '<op>' not supported on pointer operands`
   diagnostic.
6. **Tests** — basic, variable, char/short/long promotion, precedence
   vs `+`/`-`, precedence vs `<`/`>`, left-associativity, plus
   invalid pointer/array and missing-operand cases.

## Spec Issues Called Out

1. **Typo `>>>` in the long right-shift test**. The spec test for
   long-operand right shift uses `return a >>> 3;`. C has no `>>>`
   operator; that is Java's unsigned right shift. I will treat it as
   `>>` (the only right-shift operator the spec actually introduces)
   and write the test as `return a >> 3;` with the corresponding
   expected value.
2. **Missing semicolons / closing braces** in two snippets
   ("Precedence with additive expressions" and the second "Char
   promotion" example). These are presentation defects — intent is
   clear and the actual tests are written syntactically correct.
3. **Pointer/array operands on the right side**. The Requirements
   section says "Both operands must be integer types" but the
   invalid-test list only shows pointer/array on the LEFT. The
   stronger sentence governs: pointer/array operands on either side
   are rejected.
4. **Out of scope** — bad shift counts (negative, `>= width`) — left
   as undefined behavior; no diagnostic emitted.

## Planned Changes

### Tokenizer
- `include/token.h` — add `TOKEN_LEFT_SHIFT`, `TOKEN_RIGHT_SHIFT`.
- `src/lexer.c` — match `<<` before `<=` / `<` and `>>` before
  `>=` / `>`.
- `src/compiler.c` — extend `token_type_name`.

### Parser
- `src/parser.c` — new `parse_shift` between `parse_relational` and
  `parse_additive`; `parse_relational` calls `parse_shift`. Update
  grammar comments.

### AST
- No changes.

### Code Generator
- `src/codegen.c`:
  - Extend `expr_result_type` so `<<` / `>>` yield
    `promote_kind(left)`.
  - Extend `codegen_expression` `AST_BINARY_OP` arm with a new
    branch for `<<` / `>>`. Evaluate left → push, evaluate right →
    pop left into rcx; `xchg rax, rcx` so left is in rax and right
    (count) is in rcx (low byte = cl). Reject pointer operand on
    either side. Emit `shl` / `sar` against `eax` / `rax` based on
    the left's promoted type.

### Tests

Valid (`test/valid/`):
- `test_lshift_basic_1__8.c` — `return 1 << 3;`
- `test_lshift_basic_2__12.c` — `return 3 << 2;`
- `test_rshift_basic_1__4.c` — `return 16 >> 2;`
- `test_rshift_basic_2__4.c` — `return 9 >> 1;`
- `test_rshift_arith_negative__252.c` — `return -8 >> 1;`
- `test_lshift_var_int__8.c` — `int a = 1; int b = 3; return a << b;`
- `test_rshift_var_int__4.c` — `int a = 16; int b = 2; return a >> b;`
- `test_shift_precedence_additive__24.c` — `return 1 + 2 << 3;`
- `test_shift_precedence_relational__1.c` — `return 1 << 3 > 4;`
- `test_shift_left_associative__4.c` — `return 32 >> 2 >> 1;`
- `test_lshift_char_promotion__4.c` — `char a = 1; return a << 2;`
- `test_rshift_char_promotion__4.c` — `char a = 8; return a >> 1;`
- `test_lshift_short_promotion__16.c` —
  `short a = 2; return a << 3;`
- `test_lshift_long_via_call__32.c` —
  `long foo() { long a = 8L; return a << 2; } int main() { return foo(); }`
- `test_rshift_long_via_call__4.c` — long-operand right shift,
  `32L >> 3`.

Invalid (`test/invalid/`):
- `test_invalid_75__not_supported_on_pointer_operands.c` —
  `p << 1` rejected.
- `test_invalid_76__not_supported_on_pointer_operands.c` —
  `p >> 2` rejected.
- `test_invalid_77__not_supported_on_pointer_operands.c` —
  `A << 1` rejected (array decay).
- `test_invalid_78__not_supported_on_pointer_operands.c` —
  `A >> 1` rejected (array decay).
- `test_invalid_79__expected_expression.c` — `return 1 <<;`
- `test_invalid_80__expected_expression.c` — `return 1 >>;`
- `test_invalid_81__expected_expression.c` — `return >> 1;`
- `test_invalid_82__expected_expression.c` — `return << 1;`

Print-tokens (`test/print_tokens/`):
- `test_token_left_shift.c` / `.expected` — `<<` lexes as
  `TOKEN_LEFT_SHIFT`.
- `test_token_right_shift.c` / `.expected` — `>>` lexes as
  `TOKEN_RIGHT_SHIFT`.

Print-asm (`test/print_asm/`):
- `test_stage_16_03_lshift_int.c` / `.expected` — `return 1 << 3;`
  locks in `shl eax, cl`.
- `test_stage_16_03_rshift_int.c` / `.expected` — `return 16 >> 2;`
  locks in `sar eax, cl`.
- `test_stage_16_03_lshift_long.c` / `.expected` —
  `long a = 8L; return a << 2;` locks in `shl rax, cl`.

### Documentation
- `docs/grammar.md` — insert `<shift_expression>`; rewrite
  `<relational_expression>` to consume it.
- `README.md` — bump "Through stage 16-02" to "Through stage 16-03";
  mention shift operators; refresh test totals.

### Commit
Single `stage-16-03` commit covering tokenizer / parser / codegen /
tests / docs after the full suite passes. Commit trailer must
include `Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>`.

## Test Plan

```
cmake --build build
./test/run_all_tests.sh
```

Expectation: all prior tests still pass, the 15 new valid + 8 new
invalid + 2 new print-tokens + 3 new print-asm tests pass, and there
are no regressions.
