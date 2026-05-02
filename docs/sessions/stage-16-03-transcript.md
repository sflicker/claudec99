# stage-16-03 Transcript: Shift Operators

## Summary

Stage 16-03 introduces the binary shift operators `<<` and `>>` to
ClaudeC99. Both operands must be integer types (`char`, `short`,
`int`, `long`) and undergo the usual integer promotion. The result
type follows the promoted type of the left operand alone — the right
operand is a shift count and only its low byte (`cl`) is consumed.
The right shift is arithmetic (`sar`), preserving sign on negative
inputs.

A new grammar level `<shift_expression>` slots between
`<relational_expression>` (lower precedence) and
`<additive_expression>` (higher precedence), so shift binds less
tightly than `+`/`-` and more tightly than `<`/`>`/`<=`/`>=`. Pointer
and array operands on either side are rejected at codegen with the
existing
`error: operator '<op>' not supported on pointer operands`
diagnostic. Bad shift counts (negative, ≥ width) are deliberately
left unchecked — out of scope per the spec.

## Plan

The kickoff (`docs/kickoffs/stage-16-03-kickoff.md`) called out one
spec issue — the long right-shift test used `>>>`, which is not a C
operator — and resolved it by treating that as a typo for `>>`.
Implementation order: tokenizer → parser → codegen → tests → docs →
build/test, with no AST changes required.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_LEFT_SHIFT` and `TOKEN_RIGHT_SHIFT` to the `TokenType`
  enum in `include/token.h`.
- In `src/lexer.c`, matched `<<` and `>>` before the existing
  `<=` / `>=` and single-character `<` / `>` branches so the
  compound forms always win.
- Extended `token_type_name` in `src/compiler.c` so the new token
  names appear in `--print-tokens` output.

### Step 2: Parser

- Added `parse_shift` between `parse_relational` and `parse_additive`
  in `src/parser.c`. It consumes `<additive_expression> { ("<<" |
  ">>") <additive_expression> }*` and produces `AST_BINARY_OP` nodes
  with `value = "<<"` or `">>"`.
- Rewrote `parse_relational` to delegate to `parse_shift` instead of
  `parse_additive`, anchoring the new precedence level.
- Updated grammar comments above both functions.

### Step 3: AST

- No changes. Shift operators reuse `AST_BINARY_OP`.

### Step 4: Code Generator

- Extended `expr_result_type` in `src/codegen.c` so `<<` / `>>`
  report `promote_kind(expr_result_type(children[0]))` — only the
  left operand's promoted type drives the result.
- Added a new branch at the top of the `AST_BINARY_OP` arm of
  `codegen_expression`:
  - Reject pointer/array operands on either side via
    `expr_result_type` before evaluation.
  - Evaluate left → `push rax`; evaluate right → `pop rcx`;
    `xchg rax, rcx` so left is in rax and the right's low byte is
    addressable as `cl`.
  - Emit `shl`/`sar` against `eax` (left promoted to int) or `rax`
    (left is `long`).
  - Record the result type on the node.

```asm
; Example: 1 << 3
mov eax, 1
push rax
mov eax, 3
pop rcx
xchg rax, rcx
shl eax, cl
```

```asm
; Example: long a; a << 2 → 64-bit
mov rax, [rbp - 8]
push rax
mov eax, 2
pop rcx
xchg rax, rcx
shl rax, cl
```

### Step 5: Tests

- Added 15 valid tests covering basic left/right shift, arithmetic
  right shift on a negative value, variable operands, additive vs
  shift precedence, shift vs relational precedence, left-
  associativity, char/short/long promotion of the left operand, and
  the long-operand return path through a function call.
- Added 8 invalid tests: pointer-on-LHS for `<<` and `>>`, array-on-
  LHS via decay for `<<` and `>>`, and four parse-error shapes
  (`return 1 <<;`, `return 1 >>;`, `return >> 1;`, `return << 1;`).
- Added 2 print-tokens tests locking in `TOKEN_LEFT_SHIFT` and
  `TOKEN_RIGHT_SHIFT`.
- Added 3 print-asm tests locking in the emitted instruction
  sequences for 32-bit `shl`/`sar` and 64-bit `shl`.

### Step 6: Documentation

- `docs/grammar.md` — added `<shift_expression>` and rewrote
  `<relational_expression>` to consume it.
- `README.md` — bumped header to "Through stage 16-03", added a new
  bullet describing shift operators (precedence, integer-only,
  arithmetic right shift, result type follows left), and refreshed
  the test totals to 505 (308 valid, 80 invalid, 24 print-AST,
  78 print-tokens, 15 print-asm).

## Final Results

`cmake --build build` succeeds. `./test/run_all_tests.sh` reports
505/505 passing (308 valid + 80 invalid + 24 print-AST + 78
print-tokens + 15 print-asm). No regressions from earlier stages.

## Session Flow

1. Read the stage spec, supporting files, and the previous stage
   (16-02) artifacts.
2. Reviewed tokenizer / parser / codegen to plan minimal hooks.
3. Produced the kickoff summary, called out the `>>>` typo and
   missing semicolons, and saved it to `docs/kickoffs/`.
4. Implemented the tokenizer changes and verified the build.
5. Implemented the parser changes (new `parse_shift` level).
6. Implemented the codegen branch and ran the existing suite —
   all 477 prior tests still pass.
7. Smoke-tested representative shift expressions end-to-end.
8. Wrote 15 valid, 8 invalid, 2 print-tokens, and 3 print-asm tests.
9. Updated `docs/grammar.md` and `README.md`.
10. Re-ran the full suite — 505/505 pass.
11. Wrote the milestone summary and this transcript.

## Notes

- The shift operators cannot reuse the `is_arith` / `common_arith_kind`
  path from `+`/`-`/`*`/`/`/`%` because their result type depends only
  on the left operand. They are dispatched in their own branch at the
  top of the binary-op arm before the common arith / cmp logic runs.
- The `xchg rax, rcx` after `pop rcx` is the simplest way to keep the
  count addressable as `cl` while putting the left operand in rax —
  no new helper or codegen scaffolding was required.
