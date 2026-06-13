# stage-110 Transcript: float/double Arithmetic, Conversions, and Casts

## Summary

Stage 110 adds full support for floating-point arithmetic and explicit casts between all scalar types (int, float, double). The implementation extends usual arithmetic conversions (UAC) to handle mixed FP and integer operands; emits FP unary minus using xorps/xorpd with 16-byte-aligned sign-bit masks; and generates inline type conversions (cvtsi2ss, cvtsi2sd, cvtss2sd, cvtsd2ss, cvttss2si, cvttsd2si) wherever mixed operands are encountered. A critical bug was discovered and fixed during implementation: `expr_result_type` was using size-based type inference instead of `type_is_fp()` checks for local FP variables, causing the FP arithmetic codegen path to never be entered. The sign-mask operands required by xorps/xorpd are emitted on-demand to `.rodata` with strict 16-byte alignment; initial truncation to 4 bytes caused segmentation faults.

## Changes Made

### Step 1: Type System and Expression Result Type

- Added `fp_common_arith_kind(a, b)` helper in `src/codegen.c`: returns TYPE_DOUBLE if either operand is double, TYPE_FLOAT if either is float, else falls back to standard integer UAC.
- Extended `expr_result_type()` in `src/codegen.c` to add `AST_FLOAT_LITERAL` case (returns `node->decl_type` for float vs double distinction).
- Fixed critical bug in `expr_result_type()` for `AST_VAR_REF`: replaced size-based type inference (`type_kind_from_size`) with explicit `type_is_fp(lv->kind)` checks for local and global variables (float and double are both 4 and 8 bytes respectively, same as int and long, so size-based inference fails).
- Extended `expr_result_type()` for `AST_BINARY_OP` and `AST_UNARY_OP`: added FP UAC checks and FP-type propagation.
- Extended `sizeof_type_of_expr()` with same FP UAC and unary propagation fixes.

### Step 2: Type Conversion Extensions

- Extended `emit_convert()` in `src/codegen.c` to handle six new conversions:
  - int/long → float: `cvtsi2ss` (convert signed int to scalar single)
  - int/long → double: `cvtsi2sd` (convert signed int to scalar double)
  - float → double: `cvtss2sd` (convert scalar single to scalar double)
  - double → float: `cvtsd2ss` (convert scalar double to scalar single)
  - float → int: `cvttss2si` (convert with truncation scalar single to signed int)
  - double → int: `cvttsd2si` (convert with truncation scalar double to signed int)
- Extended `emit_fp_widen_if_needed()` in `src/codegen.c` to emit int→float and int→double conversions (cvtsi2ss/cvtsi2sd) in addition to float→double.
- Deployed widening at all FP assignment and initializer sites (local, global, struct member dot, struct member arrow).

### Step 3: Floating-Point Unary Minus

- Added FP unary minus handler in `codegen_expression()`: detects TYPE_FLOAT or TYPE_DOUBLE child type, emits xorps (single-precision) or xorpd (double-precision) with memory operand pointing to a sign-bit mask from `.rodata`.
- Added flags `fp_sign_mask_f32_emitted` and `fp_sign_mask_f64_emitted` to `CodeGen` struct (`include/codegen.h`) to track which masks are needed.
- Initialized both flags to 0 in `codegen_init()`.
- FP unary plus is a no-op (returns the value in xmm0).

### Step 4: Floating-Point Binary Arithmetic

- Inserted new FP arithmetic path in `codegen_expression()` before the integer arithmetic path: checks if either operand is FP type via `type_is_fp()`.
- Convention for FP binary operations:
  1. Save left-operand result (xmm0) to stack: `sub rsp,8; movss/movsd [rsp], xmm0`
  2. Evaluate right operand into xmm0
  3. Load left operand back: `movss/movsd xmm1, [rsp]; add rsp,8`
  4. For non-commutative ops (sub, div): compute `xmm1 op= xmm0` then `movss/movsd xmm0, xmm1` to restore result register
  5. Handles mixed int+FP and float+double operands by emitting cvtsi2sd/ss or cvtss2sd inline
- All eight binary operators (+, -, *, /, %, <<, >>, &, etc.) dispatch through the FP check; only +, -, *, / proceed with FP codegen.

### Step 5: Sign-Mask Emission

- Added `codegen_emit_fp_literals()` case in `src/codegen.c` to emit FP sign masks only when flags indicate they are needed:
  - `Lfp_smask_f32` (if `fp_sign_mask_f32_emitted`): `align 16; dd 0x80000000, 0, 0, 0` (16 bytes, bit 31 set)
  - `Lfp_smask_f64` (if `fp_sign_mask_f64_emitted`): `align 16; dq 0x8000000000000000, 0` (16 bytes, bit 63 set)
- Critical fix: initial emission used `dd 0x80000000` (4 bytes), causing segfault when xorps tried to read a 16-byte-aligned operand. Fixed by adding `align 16` and padding to 16 bytes.

### Step 6: Version Update

- `src/version.c`: bumped `VERSION_STAGE` from "01090000" to "01100000".

## Final Results

All 8 new test files pass:
- `test_float_add__0`: basic float addition
- `test_double_mul__0`: basic double multiplication
- `test_float_div__0`: float division
- `test_double_sub__0`: double subtraction
- `test_fp_unary_minus__0`: FP unary minus (xorps/xorpd)
- `test_fp_int_widen__0`: int-to-float widening in arithmetic
- `test_fp_cast_roundtrip__0`: explicit casts between int/float/double
- `test_fp_mixed_expr__0`: mixed operand expressions

Full test suite: 1635 passed, 0 failed, 1635 total.

Self-host results (C0→C1→C2): all 1635 tests pass at each generation. No source changes needed during bootstrap.

C0: 00.02.01100000.00872
C1: 00.02.01100000.00873
C2: 00.02.01100000.00874

## Session Flow

1. Read Stage 110 kickoff and implementation notes
2. Reviewed Stage 109 FP support to understand existing infrastructure
3. Analyzed expr_result_type() and identified size-based type-inference bug for FP locals
4. Implemented fp_common_arith_kind() and fixed expr_result_type() for FP detection
5. Extended emit_convert() and emit_fp_widen_if_needed() for six new conversions
6. Implemented FP unary minus with sign-mask emission and alignment
7. Implemented FP binary arithmetic with save/restore convention and inline conversions
8. Built and ran full test suite; all 8 new tests passed
9. Ran self-host C0→C1→C2 cycle; verified all 1635 tests pass at each step
10. Updated README.md, version, and generated milestone/transcript artifacts

## Notes

**Critical Bug Fixed**: `expr_result_type()` for local FP variables was using `type_kind_from_size(lv->size)`, which incorrectly returned TYPE_INT for 4-byte floats and TYPE_LONG for 8-byte doubles (both sizes match integer types). This prevented FP arithmetic detection. Fixed by adding explicit `type_is_fp(lv->kind)` checks before falling back to size-based inference.

**Sign-Mask Alignment**: Initial sign-mask emission used `dd 0x80000000` (4 bytes), but xorps/xorpd require 16-byte-aligned memory operands. Emission now emits `align 16` plus zero-padding to 16 bytes.

**Usual Arithmetic Conversions**: FP UAC follows C99 rules: if either operand is double, result is double; else if either is float, result is float; else use standard integer UAC. Operand widening (int→float, float→double) is emitted inline at the codegen site, not via a separate promotion pass.
