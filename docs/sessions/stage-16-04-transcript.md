# stage-16-04 Transcript: Bitwise Binary Operators

## Summary

Stage 16-04 adds the three binary bitwise operators `&` (AND), `^`
(XOR), and `|` (OR) to the compiler. All three are integer-only;
operands undergo the usual integer promotion (char/short → int;
either side long → long), and the result type follows the common
arithmetic type. Pointer or array operands on either side are
rejected at codegen with the existing
`error: operator '<op>' not supported on pointer operands` shape.

The grammar gains three new precedence levels between
`<equality_expression>` and `<logical_and_expression>`. In order of
increasing tightness: `|` < `^` < `&`, so `1 | 2 & 4` parses as
`1 | (2 & 4)` and `1 & 3 == 3` parses as `1 & (3 == 3)`. All three
are left-associative. The AST reuses `AST_BINARY_OP`; no new node
types or fields are introduced.

Codegen emits `and` / `xor` / `or` against `eax`/`ecx` (32-bit
common type) or `rax`/`rcx` (64-bit common type), reusing the
existing arithmetic-binary-op evaluation pattern: evaluate left →
push → evaluate right → pop into rcx → emit the integer instruction.
`movsxd` widening is applied to int sides when the common type is
long, matching the existing `+`/`-`/`*`/`/`/`%` widening.

## Plan

The kickoff at `docs/kickoffs/stage-16-04-kickoff.md` enumerated
expected file-level changes, called out three spec issues (the
`reutrn` typo, the literal `^&a` in the first invalid example, and
the missing result-type rule), and produced the test inventory.
Implementation order: tokenizer → parser → AST pretty printer →
codegen → tests → docs → commit, with a build at every step.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_CARET` and `TOKEN_PIPE` to `TokenType` in
  `include/token.h`.
- Extended `src/lexer.c` to emit `TOKEN_CARET` for bare `^` and
  `TOKEN_PIPE` for bare `|` (the existing `||` branch still wins
  when the second char is `|`).
- Extended `token_type_name` in `src/compiler.c` with the two new
  names so `--print-tokens` formats them.

### Step 2: Parser

- Added three new precedence levels in `src/parser.c`:
  - `parse_bitwise_and` over `parse_equality`, driven by
    `TOKEN_AMPERSAND` (the existing `&` lexeme; binary form is
    unambiguous at this point in the grammar).
  - `parse_bitwise_xor` over `parse_bitwise_and`, driven by
    `TOKEN_CARET`.
  - `parse_bitwise_or` over `parse_bitwise_xor`, driven by
    `TOKEN_PIPE`.
- Rewrote `parse_logical_and` to consume `parse_bitwise_or` instead
  of `parse_equality`, and updated the grammar comments accordingly.
- All three are left-associative and produce `AST_BINARY_OP` with
  `value` "&" / "^" / "|".

### Step 3: AST

- No new node types or fields.
- Extended `operator_name` in `src/ast_pretty_printer.c` so AST
  dumps render the three new operators as `BITAND` / `BITXOR` /
  `BITOR`, paralleling the existing `&&` → `AND` and `||` → `OR`.

### Step 4: Code Generator

- Extended `expr_result_type` in `src/codegen.c`'s `AST_BINARY_OP`
  arm: for `&` / `^` / `|`, type both operands and combine via
  `common_arith_kind`.
- Added a new branch in `codegen_expression`'s `AST_BINARY_OP` arm
  ahead of the generic arithmetic path:
  - Reject pointer operand on either side with
    `error: operator '<op>' not supported on pointer operands`.
  - Compute `common = common_arith_kind(lt, rt)`.
  - Evaluate left → optional `movsxd rax, eax` if widening to long
    → `push rax`. Evaluate right → optional `movsxd` → `pop rcx`.
  - Emit `and|xor|or rax, rcx` (long common) or
    `and|xor|or eax, ecx` (int common). Set `node->result_type`.

### Step 5: Tests

Added 30 new tests across four suites.

Valid (13 in `test/valid/`):

```
test_bitand_basic__2.c
test_bitor_basic__7.c
test_bitxor_basic__5.c
test_bitand_var_int__2.c
test_bitor_precedence_and__1.c
test_bitxor_precedence_and__0.c
test_bitand_precedence_equality__1.c
test_bitand_left_associative__1.c
test_bitor_logical_and_mixed__1.c
test_bitor_xor_and_chain__7.c
test_bitand_char_promotion__2.c
test_bitor_long_via_call__7.c
test_bitand_long_via_call__2.c
```

Invalid (11 in `test/invalid/`):

```
test_invalid_83__not_supported_on_pointer_operands.c   (p & 1)
test_invalid_84__not_supported_on_pointer_operands.c   (p | 1)
test_invalid_85__not_supported_on_pointer_operands.c   (p ^ 1)
test_invalid_86__not_supported_on_pointer_operands.c   (A & 1)
test_invalid_87__not_supported_on_pointer_operands.c   (A | 1)
test_invalid_88__not_supported_on_pointer_operands.c   (A ^ 1)
test_invalid_89__expected_expression.c                 (return 1 &;)
test_invalid_90__expected_expression.c                 (return 1 |;)
test_invalid_91__expected_expression.c                 (return 1 ^;)
test_invalid_92__expected_expression.c                 (return ^ 1;)
test_invalid_93__expected_expression.c                 (return | 1;)
```

Print-tokens (2 in `test/print_tokens/`):

```
test_token_caret.{c,expected}
test_token_pipe.{c,expected}
```

Print-asm (4 in `test/print_asm/`):

```
test_stage_16_04_bitand_int.{c,expected}    locks in `and eax, ecx`
test_stage_16_04_bitor_int.{c,expected}     locks in `or eax, ecx`
test_stage_16_04_bitxor_int.{c,expected}    locks in `xor eax, ecx`
test_stage_16_04_bitand_long.{c,expected}   locks in `and rax, rcx`
```

### Step 6: Documentation

- `docs/grammar.md` — added `<bitwise_or_expression>`,
  `<bitwise_xor_expression>`, `<bitwise_and_expression>`; rewrote
  `<logical_and_expression>` to consume `<bitwise_or_expression>`.
- `README.md` — bumped "Through stage 16-03" to "Through stage
  16-04"; added a bitwise-binary-operators bullet to the supported
  features list; refreshed the aggregate test totals
  (505 → 535 across 321 valid / 91 invalid / 24 print-AST /
  80 print-tokens / 19 print-asm).

## Final Results

```
cmake --build build
./test/run_all_tests.sh
```

Build: clean. Test results: 535/535 pass (321 valid + 91 invalid +
24 print-AST + 80 print-tokens + 19 print-asm). No regressions from
prior stages.

## Session Flow

1. Read the spec and supporting skill files; derived `STAGE_LABEL =
   stage-16-04` via the stage-label script.
2. Reviewed `lexer.c`, `parser.c`, `codegen.c`, the shift-operator
   stage 16-03 kickoff and milestone, the grammar file, and the
   README.
3. Wrote the kickoff artifact, called out the three spec issues
   (`reutrn` typo, `^&a` in the invalid example, missing
   result-type rule), and proposed the implementation order.
4. Implemented tokenizer → parser → AST printer → codegen, building
   after each step.
5. Smoke-tested the spec's expected results (`6&3`, `6|3`, `6^3`,
   `1|2&4`, `1&3==3`, `1^3&1`, `0|2&&3`).
6. Added valid, invalid, print-tokens, and print-asm tests; ran the
   full suite (535/535 pass).
7. Updated `docs/grammar.md` and `README.md`; re-ran the suite to
   confirm no regressions.
8. Wrote the milestone summary and this transcript.
9. Committed all changes under the `stage-16-04` label with the
   required co-author trailer.

## Notes

- The same `&` lexeme serves both unary address-of and binary
  bitwise AND. The parser disambiguates by position:
  `parse_bitwise_and` only sees `&` after a primary/postfix
  expression has been consumed, so the binary form is unambiguous
  there.
- `&`, `^`, and `|` are commutative, so the existing arithmetic
  pattern of having LHS in rcx and RHS in rax after the pop is
  fine for these operators — no swap is required.
- Bad shift counts and unsigned bitwise behavior remain out of
  scope, consistent with the prior stages.
