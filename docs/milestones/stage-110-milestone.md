# Milestone Summary

## Stage 110 — float/double Arithmetic, Conversions, and Casts: Complete

Stage 110 ships floating-point arithmetic and conversions: UAC for mixed FP types, FP unary minus, binary arithmetic (add/sub/mul/div) with mixed int/float operand handling, and explicit casts between all scalar types.

- Tokenizer: No changes (float/double already in Stage 109).
- Grammar/Parser: No changes (arithmetic and cast expressions use existing rules).
- AST/Semantics: Extended `expr_result_type` to detect FP operands via `type_is_fp()` instead of size-based inference; fixed bug where float/double locals returned TYPE_INT/TYPE_LONG; added `fp_common_arith_kind()` helper for FP+int UAC.
- Codegen: FP unary minus (xorps/xorpd with 16-byte-aligned sign mask), FP binary arithmetic (save/restore xmm0 across operands, emit cvtsi2ss/cvtsi2sd/cvtss2sd/cvtsd2ss/cvttss2si/cvttsd2si conversions inline), `emit_fp_widen_if_needed()` extended for int→float/double, sign-mask literals emitted on demand to `.rodata`.
- Tests: 8 new valid tests (float/double arithmetic and casts). Full suite 1635 passed, 0 failed, 1635 total.
- Docs: README updated; grammar unchanged (no syntax additions); checklist updated.
- Notes: Bug found and fixed during implementation — `expr_result_type` for local FP vars incorrectly used size-based type inference, returning TYPE_INT/LONG instead of TYPE_FLOAT/DOUBLE, preventing the FP arithmetic codegen path from being entered. xorps/xorpd require 16-byte-aligned memory operands; initial emission was truncated, causing SIGSEGV. Self-host C0→C1→C2 passes with no source changes needed during bootstrap.
