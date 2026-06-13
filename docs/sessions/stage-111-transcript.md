# stage-111 Transcript: Float/Double Comparisons and Boolean Contexts

## Summary

Stage 111 enables floating-point comparisons and boolean contexts, allowing `float` and `double` values to be used with all relational and equality operators, in control-flow conditions (`if`, `while`, `for`, ternary `?:`), and with the logical NOT operator. All comparison operations follow IEEE 754 unordered-compare semantics via SSE2 `ucomisd`/`ucomiss` instructions. Mixed int/FP comparisons automatically promote the integer operand inline. NaN is treated as true in a bare boolean context and follows C99 semantics in comparisons (`!=` returns 1 for NaN, all other comparisons return 0). This is the final stage of the floating-point implementation plan; the compiler now has complete FP support except for function parameters/return values and variadic FP arguments (Stage 112).

## Changes Made

### Step 1: Add FP Boolean Context Helper

- Created `emit_fp_bool_to_rax()` function in `src/codegen.c` (placed in FP helper section after `emit_fp_widen_if_needed` to avoid C99 implicit-declaration error).
- Converts FP value in `xmm0` (float or double) to 0 or 1 in `rax`.
- Uses `xorpd xmm1, xmm1` (or `xorps` for float) to zero the comparison register.
- Emits `ucomisd xmm0, xmm1` (or `ucomiss` for float).
- Applies flag-based assembly: `setne al` (value != 0), `setp cl` (NaN?), `or al, cl` (result is 1 if nonzero or NaN), `movzx rax, al`.
- Correctly treats NaN as true (per ClaudeC99 policy for bare boolean contexts).

### Step 2: FP Comparison Codegen in Binary Operations

- Added new block in `AST_BINARY_OP` case (after FP arithmetic block, guarded by `type_is_fp()` on either operand).
- Handles comparison operators: `<`, `<=`, `>`, `>=`, `==`, `!=` when either operand is FP.
- Load-operand sequence: evaluate left into `xmm0`, push to stack (`sub rsp, 8; movss/movsd [rsp], xmm0`), evaluate right into `xmm0`, restore left into `xmm1` (`movss/movsd xmm1, [rsp]; add rsp, 8`).
- Emit `ucomiss xmm1, xmm0` (float) or `ucomisd xmm1, xmm0` (double).
- Flag-based assembly per operator:
  - `<`: `setb al` (CF=1)
  - `<=`: `setbe al` (CF=1 or ZF=1)
  - `>`: `seta al` (CF=0 and ZF=0)
  - `>=`: `setae al` (CF=0)
  - `==`: `sete al; setnp cl; and al, cl` (equal AND not unordered)
  - `!=`: `setne al; setp cl; or al, cl` (not-equal OR unordered)
- Final: `movzx rax, al`; set `result_type = TYPE_INT`.
- Handles mixed int/FP via existing `fp_common_arith_kind()` UAC that promotes integer side inline via `cvtsi2ss`/`cvtsi2sd`.

### Step 3: Logical NOT on FP Operands

- Added FP branch in unary `!` operator handling (before integer `!` path).
- Detects FP child type via `type_is_fp(operand->result_type)`.
- Emits same assembly as FP boolean context: `xorpd/xorps`, `ucomisd/ucomiss`, `sete al`, `setnp cl`, `and al, cl`, `movzx rax, al`.
- Semantics: `!0.0 = 1`, `!NaN = 0`, `!any_other_fp = 0` (per C99 §6.5.3.3: NOT of unordered value returns 0).

### Step 4: FP Condition Handling in Control Flow

- Extended `emit_cond_cmp_zero()` helper: added FP branch check at top (calls `emit_fp_bool_to_rax()` if condition is FP type, then falls through to existing `cmp eax, 0` path).
- Updated `AST_CONDITIONAL_EXPR` condition handling with same FP check.
- Enables `if (fp_value)`, `while (fp_expr)`, `for (...; fp_cond; ...)`, and `condition_fp ? true_expr : false_expr`.

### Step 5: Version Bump

- Updated `src/version.c` to set `VERSION_STAGE = "01110000"` (Stage 111 binary: 0111 = 7, 0000 = 0 padding).

### Step 6: Test Suite

- Added 8 new valid tests in `test/valid/`:
  - `test_fp_less_than__0.c`: `<` comparison (double 1.0 < 2.0 → true).
  - `test_fp_equal__0.c`: `==` comparison (float 4.0 == 2.0+2.0 → true).
  - `test_fp_not_equal__0.c`: `!=` comparison (double 1.5 != 2.5 → true).
  - `test_fp_if_condition__0.c`: FP value in `if` (double 3.14 → taken).
  - `test_fp_while_loop__0.c`: FP counter in loop (sum 1..10 over doubles → 55).
  - `test_fp_logical_not__0.c`: `!` on FP (zero vs nonzero → correct negation).
  - `test_fp_ternary__0.c`: FP value in ternary condition (0.5 > 0.0 → 42).
  - `test_fp_mixed_cmp__0.c`: FP compared against int (5.0 > 3 → true).

## Final Results

All 1643 tests pass (1635 prior + 8 new). Self-host C0→C1→C2 cycle verified:
- C0: `00.02.01110000.00881` (GCC-built), 1643/1643 tests pass.
- C1: `00.02.01110000.00882` (C0-built), 1643/1643 tests pass.
- C2: `00.02.01110000.00883` (C1-built), 1643/1643 tests pass.

No regressions; bootstrap produces fixed point (C1 and C2 are behavior-identical).

## Session Flow

1. Read spec, floating-point-plan.md, and Stage 110 milestone for context.
2. Analyzed `ucomiss`/`ucomisd` instruction semantics and flag patterns.
3. Planned four implementation areas: helper function, binary comparisons, unary NOT, control-flow conditions.
4. Implemented `emit_fp_bool_to_rax()` helper after `emit_fp_widen_if_needed` (forward-declaration fix).
5. Added FP comparison block in `AST_BINARY_OP` with save/restore and `ucomiss`/`ucomisd` emission.
6. Added FP NOT in unary operator handling.
7. Extended condition-handling sites (`emit_cond_cmp_zero`, `AST_CONDITIONAL_EXPR`).
8. Updated version string to Stage 111.
9. Created 8 new integration tests covering all comparison operators, boolean contexts, and mixed-type operands.
10. Built and ran full test suite (1643 tests); verified self-host C0→C1→C2 cycle; all tests pass.

## Notes

### Forward Declaration Resolution

The `emit_fp_bool_to_rax()` helper was initially placed at the end of the code-generator file, just before its first caller in the unary NOT handler. This triggered a C99 implicit-function-declaration error during bootstrap compilation of the compiler itself. The fix was to place the function definition in the FP helper section immediately after `emit_fp_widen_if_needed`, ensuring it is declared before `codegen_expression()` invokes it.

### Operand Order After Save/Restore

Following the same convention established in Stage 110, the left operand is saved to the stack, the right operand is evaluated into `xmm0`, and the left is restored into `xmm1`. This inverted state (right in `xmm0`, left in `xmm1`) matches the operand order for `ucomisd xmm1, xmm0` (left=xmm1, right=xmm0).

### Logical NOT vs. Plan

The floating-point-plan.md originally described logical NOT on FP as "same as boolean context, then `xor al, 1`." The actual implementation uses `sete + setnp + and` from the spec, which is cleaner and gives identical semantics for normal values while correctly producing `!NaN = 0` (the XOR approach would produce `!NaN = 1`, incorrect per C99).

### NaN Handling

- In comparisons (`<`, `<=`, `>`, `>=`, `==`): all return 0 except `!=` (returns 1).
- In logical NOT: `!NaN = 0` (via `sete + setnp + and`).
- In bare boolean context (`if (fp_val)`): NaN is treated as true (via `setne + setp + or`). This is a valid C99 implementation choice; C99 §6.5.8 notes the unordered case is implementation-defined for bare conditions.
