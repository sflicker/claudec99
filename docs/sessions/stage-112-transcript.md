# stage-112 Transcript: Floating-Point Calling Convention, va_arg for Double, and math.h

## Summary

Stage 112 completes floating-point support through the SysV AMD64 ABI calling convention. FP arguments (float/double) go in xmm0–xmm7, tracked independently from GP arguments in rdi–r9. Non-variadic function prologues move incoming xmmN values to local stack slots; variadic prologues save xmm0–xmm7 to a 176-byte register-save area (48 GP + 128 XMM). The `al` register is set to the XMM argument count before variadic calls. `va_arg(ap, double)` fetches from the register-save area or overflow-arg-area based on fp_offset; `va_arg(ap, float)` is rejected per C99 §6.5.2.2p6 because float is always promoted to double in variadic calls. A `test/include/math.h` stub header enables test programs to call libm functions (sqrt, pow, sin, cos, etc.). Three bugs were discovered during the self-host bootstrap cycle and fixed: (1) `movaps` alignment (register-save-area offset 184→176 to place xmm0 at [rbp-128], 16-byte aligned); (2) variadic extra arguments misclassified as GP (fixed by computing expr_result_type before compute_call_layout); (3) arithmetic in local array dimension ([MAX_CALL_LAYOUT_ITEMS + 2]→[26], literal required by ClaudeC99).

## Changes Made

### Step 1: Code Generator — FP Argument Classification

- Extended `compute_call_layout` to accept `const TypeKind *actual_types` array (computed from expr_result_type for each argument).
- Updated argument classification loop to track `xmm_arg_count` and `gp_arg_count` separately; FP types assigned to xmm0–xmm7, others to rdi–r9.
- Variadic extra arguments now classified correctly based on actual expression type, not decl_type.

### Step 2: Code Generator — Non-Variadic FP Prologues

- Function entry prologue: for each non-variadic FP parameter, emit `movss [rbp - offset], xmmN` (float) or `movsd [rbp - offset], xmmN` (double) to load from incoming xmmN into local stack slot.
- Track incoming xmm parameter index alongside gp parameter index.

### Step 3: Code Generator — Variadic FP Prologues

- Variadic function prologue: allocate 176-byte register-save area (48 bytes GP + 128 bytes XMM; exact fit when rbp mod 16 = 0 after call + push rbp = 16 bytes, placing xmm0 at [rbp-128] mod 16 = 0).
- Save rdi–r9 to [reg_save_area + 0..48) as before.
- Save xmm0–xmm7 to [reg_save_area + 48..176) using `movaps [rsp + N], xmmN` (16-byte aligned).
- Initialize va_list: `fp_offset = 48 + named_xmm_params * 16`; update gp_offset and overflow_arg_area accordingly.

### Step 4: Code Generator — Variadic Call Sites

- Before variadic function call, set `al` to the XMM argument count (changed from `xor eax, eax`).
- Emit `mov al, <xmm_arg_count>` to satisfy SysV AMD64 ABI protocol.

### Step 5: Code Generator — va_arg for double

- `AST_BUILTIN_VA_ARG` codegen: for double, read `ap->fp_offset` from va_list.
- If `fp_offset < 176`: load from `reg_save_area + fp_offset`, increment `fp_offset += 16`.
- Else: load from `overflow_arg_area` (standard overflow-arg path).
- For float: emit compile error (C99 requires double; float is promoted).

### Step 6: va_start Initialization

- `__builtin_va_start` codegen: compute `fp_offset = 48 + (count of named FP parameters) * 16`.
- Add `variadic_named_xmm_params` field to CodeGen struct to track named xmm parameters in function prologue.

### Step 7: Test Infrastructure — .libs Companion Support

- Updated `test/valid/run_tests.sh` to recognize and process `.libs` companion files.
- `.libs` file contains space-separated linker flags (e.g., `-lm` for math library).
- Passed to `gcc` link command: `gcc -no-pie obj.o -o exe -lm`.

### Step 8: Test Headers — math.h Stub

- Created `test/include/math.h` with declarations for: sin, cos, tan, asin, acos, atan, atan2, sinh, cosh, tanh, exp, log, log2, log10, pow, sqrt, cbrt, fabs, fabsf, floor, floorf, ceil, ceilf, round, roundf, fmod, fmodf, fmin, fmax, fminf, fmaxf, isnan, isinf, isfinite.
- Single-precision variants use `f` suffix (e.g., `sinf`, `sqrtf`).
- Double-precision (no suffix) declarations for all functions.
- Constants: M_PI, M_E, M_SQRT2.

### Step 9: Tests — New FP Calling Tests

- `test_fp_func_call__0.c` — direct FP function call (float and double).
- `test_fp_mixed_params__0.c` — mixed int/FP function parameters.
- `test_fp_return_float__0.c` — function returning float.
- `test_fp_sqrt__0.c` — calls `sqrt` from libm (`.libs` file with `-lm`).
- `test_fp_printf__0.c` — printf with `%f` FP format (`.expected` file for output validation).
- `test_fp_varargs__0.c` — variadic function with double arguments; `va_arg` extraction.
- `test_fp_pow__0.c` — calls `pow` from libm (`.libs` file with `-lm`).

## Final Results

All 1650 tests pass (965 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). No regressions. Self-host C0→C1→C2 cycle verified after three bootstrap bugs fixed.

## Session Flow

1. Reviewed stage 112 spec and floating-point plan.
2. Identified bootstrap defects during self-host cycle.
3. Fixed bug 1: movaps alignment (rso 184→176).
4. Fixed bug 2: compute_call_layout expr_result_type for variadic extras.
5. Fixed bug 3: local array dimension arithmetic ([MAX_CALL_LAYOUT_ITEMS + 2]→[26]).
6. Implemented FP prologue and va_arg support.
7. Added math.h stub and test infrastructure (.libs support).
8. Ran full test suite; verified all 1650 pass.
9. Completed self-host C0→C1→C2 cycle.
10. Generated milestone and transcript documentation.

## Implementation Notes

### Bug 1: movaps Alignment

Initial register-save-area offset (rso) was 304 bytes. When reduced to handle only 48 GP + 128 XMM = 176 bytes, the xmm0 slot landed at [rbp - 136], which is 8 mod 16 (unaligned). `movaps` requires 16-byte alignment. Fixed by setting rso = 176, placing xmm0 at [rbp - 128] (0 mod 16, aligned). Verification: `rbp mod 16 = 0` (after call instruction + `push rbp` = 16 bytes); [rbp - 128] mod 16 = 0.

### Bug 2: compute_call_layout decl_type

For variadic function calls, extra arguments (beyond fixed parameters) had `decl_type = 0` (TYPE_VOID) from the expression node, causing them to be misclassified as GP args. Fix: compute `expr_result_type` for each argument before calling `compute_call_layout`, passing the actual types array instead of relying on decl_type.

### Bug 3: Local Array Dimension Arithmetic

ClaudeC99 does not support arithmetic expressions in local array declarator sizes. A provisional line `ArgSlot *layout = ...malloc(...[MAX_CALL_LAYOUT_ITEMS + 2])` failed during bootstrap because the compiler's own parser rejected the arithmetic. Fixed by using the literal `[26]` (FUNC_MAX_PARAMS + 2 headroom, with MAX_CALL_LAYOUT_ITEMS = 24 and 2 extra = 26).
