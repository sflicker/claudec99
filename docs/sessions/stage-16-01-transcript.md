# stage-16-01 Transcript: Remainder Operator

## Summary

Stage 16-01 adds the binary `%` (remainder) operator across the
pipeline. The lexer recognizes `%` as a new `TOKEN_PERCENT`; the
parser accepts it inside multiplicative expressions with the same
left-associativity and precedence as `*` and `/`; and the code
generator emits `idiv` and reads the remainder out of `edx`
(32-bit) or `rdx` (64-bit). The AST is unchanged — `%` reuses the
existing `AST_BINARY_OP` shape with `value = "%"`.

`%` is integer-only. Char/short operands promote to `int` and
mixed `int` / `long` widens to `long`, both via the same path
that `*` and `/` already use. Pointer operands and array names
(which decay to pointers in value contexts) are rejected by the
pre-existing pointer-operator guard, with no new code path needed.
Division-by-zero is not checked in this stage, per spec.

## Plan

The kickoff identified the work as additive rather than
restructuring: a new token, one extra arm in the multiplicative
loop, two new emission cases in codegen (one for `TYPE_LONG`, one
for `TYPE_INT`), and ten valid + five invalid tests modeled on
the spec's test list. The kickoff also flagged the spec's
expected value for `2 % 4 % 2` (`2`) as inconsistent with both
left-associativity (`(2 % 4) % 2 = 0`) and right-associativity
(`2 % 0` is undefined). The test source was kept verbatim and
the expected exit code was set to `0`.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_PERCENT` to the `TokenType` enum in
  `include/token.h`.
- Added a single-character `%` branch to `lexer_next_token` in
  `src/lexer.c`, sibling to `*` and `/`.
- Added `case TOKEN_PERCENT: return "TOKEN_PERCENT";` to
  `token_type_name` in `src/compiler.c` so `--print-tokens`
  recognizes the new token.

### Step 2: Parser

- Extended the `parse_term` loop in `src/parser.c` to also
  accept `TOKEN_PERCENT`. The grammar comment was updated to
  read `("*" | "/" | "%")`.

### Step 3: AST

- No changes (`%` reuses `AST_BINARY_OP` with `value = "%"`).

### Step 4: Code Generator

- Extended the arithmetic-operator classifier in
  `expr_result_type` (`src/codegen.c`) to include `%`, so the
  result type is computed via `common_arith_kind` like the
  other multiplicative operators.
- Extended `is_arith` in `codegen_expression` to include `%`,
  so left/right widening to `TYPE_LONG` and the pointer
  rejection path apply.
- Added a 64-bit emission branch:
  ```asm
  xchg rax, rcx
  cqo
  idiv rcx
  mov rax, rdx
  ```
- Added a 32-bit emission branch:
  ```asm
  xchg eax, ecx
  cdq
  idiv ecx
  mov eax, edx
  ```
- The pre-existing
  `error: operator '%' not supported on pointer operands`
  guard now fires for both `p % b` (pointer operand) and
  `A % b` (array decay) without further code.

### Step 5: Tests

Valid (10 added in `test/valid/`):

- `test_remainder_basic_zero__0.c` — `return 1 % 1;`
- `test_remainder_basic_one__1.c` — `return 5 % 4;`
- `test_remainder_basic_two__2.c` — `return 20 % 6;`
- `test_remainder_precedence_mul__1.c` — `return 1 + 2 % 2 * 6;`
- `test_remainder_precedence_div__2.c` — `return 1 + 5 % 3 / 2;`
- `test_remainder_left_assoc__0.c` — `return 2 % 4 % 2;`
- `test_remainder_variables__1.c` — `int a = 5; int b = 4;
  return a % b;`
- `test_remainder_in_if__1.c` — `if (a % b)` falls through to
  `else { return 1; }` when `a = 4, b = 2`.
- `test_remainder_in_while__5.c` — `while (a % b) { rtn++; b++; }`
  with `a = 7, b = 2`.
- `test_remainder_long__2.c` — `long a = 20L; long b = 6L;
  return a % b;`

Invalid (5 added in `test/invalid/`):

- `test_invalid_66__expected_expression.c` — `return %;`
- `test_invalid_67__expected_expression.c` — `return 1%;`
- `test_invalid_68__expected_expression.c` — `return %1;`
- `test_invalid_69__not_supported_on_pointer_operands.c` —
  `p % b` with `int *p`.
- `test_invalid_70__not_supported_on_pointer_operands.c` —
  `A % b` with `int A[8]` (relies on array-to-pointer decay).

Print-tokens (1 added in `test/print_tokens/`):

- `test_token_percent.c` / `.expected` — a bare `%` lexes as
  `TOKEN_PERCENT`, mirroring the existing
  `test_token_star.c` / `test_token_slash.c` shape.

Print-asm (2 added in `test/print_asm/`):

- `test_stage_16_01_remainder_int.c` / `.expected` — `return
  20 % 6;` locks in the 32-bit `xchg eax, ecx; cdq; idiv ecx;
  mov eax, edx` sequence.
- `test_stage_16_01_remainder_long.c` / `.expected` — `long a,
  b; return a % b;` locks in the 64-bit `xchg rax, rcx; cqo;
  idiv rcx; mov rax, rdx` sequence.

### Step 6: Documentation

- `docs/grammar.md` — `<multiplicative_expression>` now lists
  `%` alongside `*` and `/`.
- `README.md` — header bumped to "Through stage 16-01"; new
  "Arithmetic operators" bullet describes the supported set
  including `%`; aggregate test totals refreshed
  (461 = 284 + 68 + 24 + 75 + 10).
- `docs/kickoffs/stage-16-01-kickoff.md`,
  `docs/milestones/stage-16-01-milestone.md`, and this
  transcript.

## Final Results

All 461 tests pass (443 prior + 10 new valid + 5 new invalid +
1 new print-tokens + 2 new print-asm = 461). No regressions.
Per-suite: 284 valid, 68 invalid, 24 print-AST, 75 print-tokens,
10 print-asm.

## Session Flow

1. Read the stage-16-01 spec and the existing multiplicative
   path through tokenizer / parser / codegen.
2. Compared `%` against the existing `*` / `/` paths to
   determine which structures could be reused (all of them) and
   which needed extension (the operator classifier in two
   places and the emission switch).
3. Wrote the kickoff summary, calling out the spec issue with
   `2 % 4 % 2`'s expected value and the diagnostic-fragment
   choices for the invalid tests.
4. Implemented the tokenizer, parser, and codegen changes.
5. Built the compiler and smoke-tested basic, long, precedence,
   left-associativity, pointer-operand, array-operand, and
   missing-operand cases against the binary directly.
6. Added the 10 valid and 5 invalid tests.
7. Ran the full suite; all 458 tests passed.
8. Updated `docs/grammar.md` and `README.md`, then wrote the
   milestone summary and this transcript.

## Notes

- The remainder emission deliberately mirrors the existing
  `/` path (which already handles the `xchg` and `cdq`/`cqo`
  setup) and only differs in the final `mov` that copies the
  remainder out of `edx` / `rdx` into the result register.
- Array operands are rejected via the same diagnostic as
  pointer operands because the array-to-pointer decay rule from
  Stage 13-03 means `A` is already a pointer by the time it
  reaches the binary-op type check. No array-specific guard was
  added.
- The spec's `2 % 4 % 2 // expect 2` is mathematically
  inconsistent with C's left-associative `%`. The test source
  was kept verbatim (`return 2 % 4 % 2;`) and the expected exit
  code uses the actual left-associative value `0`.
