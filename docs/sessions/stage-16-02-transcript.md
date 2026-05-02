# stage-16-02 Transcript: Unary Bitwise Complement

## Summary

Stage 16-02 adds the unary bitwise complement operator `~` across the
pipeline. The lexer recognizes `~` as a new `TOKEN_TILDE`; the parser
accepts it inside `<unary_expr>` alongside `+`, `-`, `!`, `++`, `--`,
`*`, and `&`; and the code generator emits `not eax` (32-bit) or
`not rax` (64-bit) after promoting the operand. The AST is unchanged
— `~` reuses the existing `AST_UNARY_OP` shape with `value = "~"`.

`~` is integer-only. Char/short operands sign-extend to `int` on load
(no extra promotion code), `int` operands stay 32-bit, and `long`
operands stay 64-bit. Pointer and array operands are rejected with
the diagnostic `error: operator '~' not supported on pointer
operands`, mirroring the pattern used by `%` in stage 16-01. Beyond
the stage spec, `!` was also tightened in this stage to reject
pointer/array operands so the two integer-only unary operators share
a single rule.

## Plan

The kickoff identified the work as additive: a new token, one extra
arm in `parse_unary`, two new emission cases in codegen (one for
`TYPE_LONG`, one for promoted-int), plus pointer-rejection
guards on both `~` and `!`. The user explicitly directed the `!`
tightening at kickoff time as a scope addition beyond the stage spec.
Test plan covered the spec's basic / nested / variable / promotion /
precedence cases plus pointer- and array-operand rejection for both
`~` and `!`.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_TILDE` to the `TokenType` enum in `include/token.h`.
- Added a single-character `~` branch to `lexer_next_token` in
  `src/lexer.c`, sibling to `%`.
- Added `case TOKEN_TILDE: return "TOKEN_TILDE";` to
  `token_type_name` in `src/compiler.c` so `--print-tokens`
  recognizes the new token.

### Step 2: Parser

- Extended `parse_unary` in `src/parser.c` to also accept
  `TOKEN_TILDE`. The grammar comment was updated to read
  `("+" | "-" | "!" | "~" | "++" | "--" | "*" | "&")`.

### Step 3: AST

- No changes (`~` reuses `AST_UNARY_OP` with `value = "~"`).

### Step 4: Code Generator

- Extended the `AST_UNARY_OP` arm in `expr_result_type`
  (`src/codegen.c`) so `~` is treated like `+`/`-`: result type is
  `promote_kind(operand)`.
- Extended the `AST_UNARY_OP` emission branch in
  `codegen_expression`:
  - New `~` case rejects `TYPE_POINTER` operand with
    `error: operator '~' not supported on pointer operands`. For
    `TYPE_LONG` operand emits `not rax` and sets result type to
    `TYPE_LONG`. Otherwise emits `not eax` and sets result type to
    `TYPE_INT`.
  - Tightened existing `!` case to reject `TYPE_POINTER` operand
    with `error: operator '!' not supported on pointer operands`.
- The pre-existing array-to-pointer decay from stage 13-03 means
  `!A` and `~A` for `int A[N]` reach the new guards as pointer
  operands and are rejected without an array-specific code path.

### Step 5: Tests

Valid (9 added in `test/valid/`):

- `test_complement_basic_neg3__253.c` — `return ~2;`
- `test_complement_basic_neg1__255.c` — `return ~0;`
- `test_complement_basic_zero__0.c` — `return ~-1;`
- `test_complement_nested__5.c` — `return ~~5;`
- `test_complement_var_int__253.c` — `int a = 2; return ~a;`
- `test_complement_var_char__253.c` — `char a = 2; return ~a;`
- `test_complement_var_short__253.c` — `short a = 2; return ~a;`
- `test_complement_var_long__253.c` —
  `long f() { long a = 2L; return ~a; } int main() { return f(); }`
- `test_complement_precedence__1.c` — `return ~2 + 4;`

Invalid (4 added in `test/invalid/`):

- `test_invalid_71__not_supported_on_pointer_operands.c` — `~p`
  with `int *p`.
- `test_invalid_72__not_supported_on_pointer_operands.c` — `~A`
  with `int A[2]` (relies on array decay).
- `test_invalid_73__not_supported_on_pointer_operands.c` — `!p`
  with `int *p` (`!` tightening).
- `test_invalid_74__not_supported_on_pointer_operands.c` — `!A`
  with `int A[2]` (`!` tightening, array decay).

Print-tokens (1 added in `test/print_tokens/`):

- `test_token_tilde.c` / `.expected` — a bare `~` lexes as
  `TOKEN_TILDE`, mirroring the existing `test_token_bang.c` /
  `test_token_percent.c` shape.

Print-asm (2 added in `test/print_asm/`):

- `test_stage_16_02_complement_int.c` / `.expected` — `return ~2;`
  locks in the 32-bit `not eax` sequence.
- `test_stage_16_02_complement_long.c` / `.expected` —
  `long f() { long a = 2L; return ~a; }` locks in the 64-bit
  `not rax` sequence.

### Step 6: Documentation

- `docs/grammar.md` — `<unary_operator>` now lists `~` alongside
  `+`, `-`, `!`, `++`, `--`, `*`, and `&`.
- `README.md` — header bumped to "Through stage 16-02"; new
  "Unary operators" bullet describes the supported set including
  `~` and the integer-only restriction on `~` and `!`; aggregate
  test totals refreshed (477 = 293 + 72 + 24 + 76 + 12).
- `docs/kickoffs/stage-16-02-kickoff.md`,
  `docs/milestones/stage-16-02-milestone.md`, and this transcript.

## Final Results

All 477 tests pass (461 prior + 9 new valid + 4 new invalid + 1 new
print-tokens + 2 new print-asm = 477). No regressions. Per-suite:
293 valid, 72 invalid, 24 print-AST, 76 print-tokens, 12 print-asm.

## Session Flow

1. Read the stage-16-02 spec and the existing unary path through
   tokenizer / parser / codegen, then the stage-16-01 kickoff /
   milestone for the most recent precedent.
2. Compared `~` against the existing `+`/`-`/`!` paths and the `%`
   pointer-rejection pattern from stage 16-01 to determine the
   minimum surface area for the change.
3. Wrote the kickoff summary; user directed adding pointer-operand
   rejection to `!` as well, beyond the spec, and the kickoff was
   updated.
4. Implemented tokenizer, parser, and codegen changes.
5. Built the compiler and smoke-tested basic, long, char/short
   promotion, nested, precedence, and pointer-operand rejection
   cases against the binary directly.
6. Added the 9 valid, 4 invalid, 1 print-tokens, and 2 print-asm
   tests.
7. Ran the full suite; all 477 tests passed.
8. Updated `docs/grammar.md` and `README.md`, then wrote the
   milestone summary and this transcript.

## Notes

- The `~` emission deliberately mirrors the existing unary `-`
  path: result type comes from `promote_kind(operand)` and the
  instruction width follows the result type (32-bit `not eax`
  vs. 64-bit `not rax`). Char/short already sign-extend to `int`
  on load.
- Array operands are rejected by the same diagnostic as pointer
  operands because the array-to-pointer decay rule from stage
  13-03 means `A` is already a pointer by the time it reaches the
  unary-op type check. No array-specific guard was added.
- The `!` pointer-rejection tightening is not in the stage 16-02
  spec; it was added on user direction so the integer-only unary
  operators (`~` and `!`) share a single rule. The existing
  positive-side `!` tests (`test_unary_bang_*`, `test_bang_long_*`)
  use only integer operands and are unaffected.
