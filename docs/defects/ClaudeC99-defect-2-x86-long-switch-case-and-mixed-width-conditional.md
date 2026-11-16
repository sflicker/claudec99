# ClaudeC99 defect 2 — x86: 64-bit `case` values and mixed-width `?:`

**Found:** 2026-11-15, while planning Stage 183 (ARM64 Phase 3). Candidate tests
for the ARM64 backend were also run on the `x86_64-macos` target.
**Affects:** the x86 backend (`src/codegen.c`), on both `x86_64-linux` and
`x86_64-macos`. The ARM64 backend is not affected; Stage 183 implements both
cases correctly there.
**Status:** open. Planned as a small x86 fix stage after Stage 183.

## Bug A — `switch` on a `long` with a `case` value outside the 32-bit range

```c
int main(void) {
    long v = 4294967296L + 7;   /* 2^32 + 7 */
    switch (v) {
    case 7:           return 1;   /* would match if v were truncated */
    case 4294967303L: return 7;
    default:          return 2;
    }
}
```

- **Expected exit code:** 7. **x86 result:** 2.
- **Cause:** the switch dispatch in `codegen_statement` (the `AST_SWITCH_STATEMENT`
  branch) emits `cmp rax, <case value>` directly. `cmp r64, imm` only accepts a
  sign-extended 32-bit immediate, so NASM warns `signed dword exceeds bounds` and
  encodes a truncated value.
- **Likely fix:** for 64-bit discriminants whose case value is outside
  `INT32_MIN..INT32_MAX`, load the value first (`mov rcx, <value>`, then
  `cmp rax, rcx`).

## Bug B — `?:` with an `int` branch and a `long` branch is not sign-extended

```c
int main(void) {
    int  c = 1;
    long r = c ? -1 : 5L;   /* result type is long; -1 must become -1L */
    return r == -1;
}
```

- **Expected exit code:** 1. **x86 result:** 0.
- **Scope:** checking the upper 32 bits of `r` shows they are 0, where clang gives
  all ones. This happens with a literal or a variable `int` branch, and with the
  `int` operand on either side.
- **Cause (not yet investigated in detail):** the `AST_CONDITIONAL_EXPR` branch in
  `codegen_expression` normalizes floating-point branches (Stage 114) but does not
  widen an `int` branch to the common integer type before the join.
- **Likely fix:** when the common type is 64-bit and a branch's type is 32-bit,
  emit `movsxd rax, eax` (or `cdqe`) on that branch before the jump or the end
  label.

## Tests to add with the fix

These are target-neutral `test/valid` tests. They were dropped from the Stage 183
plan because they fail on x86:

- `control_flow/test_switch_long_discriminant__7.c` (Bug A, the program above)
- `expressions/test_conditional_mixed_width__1.c` (Bug B, the program above)

When the fix lands, add both to `test/valid/arm64_tiers.txt` at tier 3.
