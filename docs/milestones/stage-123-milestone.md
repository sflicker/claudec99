# Milestone Summary

## Stage 123: ABI Bug Fixes — FP Stack Args, Indirect FP Calls, Address Constants, FP Short-Circuit - Complete

stage-123 fixes five correctness bugs reported in `docs/defects/CC99_FIX_REPORT-02.md`.

- Tokenizer: No changes.
- Grammar/Parser: Extended global pointer initializer validation to accept function designators (`AST_VAR_REF`) and address-of expressions (`AST_ADDR_OF`) in addition to integer/string literals.
- AST/Semantics: No changes.
- Codegen:
  - **CC99-001 & CC99-002 (stack-passed FP args)**: Added `is_fp` field to `ArgSlot` struct; set it in `compute_call_layout`. Phase 1 of the direct-call emission path now uses `movss`/`movsd` (not `mov rax`) for floating-point stack arguments (those beyond the 8 XMM registers). This also fixes variadic `va_arg(ap, double)` for the 9th and later double arguments, since the same caller-side store was wrong.
  - **CC99-003 (indirect FP calls)**: The `AST_INDIRECT_CALL` handler now detects when any argument is a floating-point type and uses a `CallLayout`-based spill/restore path (matching the direct call's `involves_special` path) instead of the old GP-register-only push/pop path.
  - **CC99-004 (address-constant initializers)**: `codegen_add_global` stores `AST_ADDR_OF` init nodes for pointer globals; `codegen_emit_data` emits `dq label` or `dq label + offset` for `&global` and `&global[N]` forms. Block-scope static pointers detect address-constant initializers before calling `eval_const_init`; `LocalStaticVar` gained `is_label_init`/`init_label` fields; `codegen_emit_local_statics` emits the corresponding `.data` entries.
  - **CC99-005 (FP logical short-circuit)**: The `&&` and `||` handlers now call `emit_fp_bool_to_rax` when the operand result type is FP before the conditional branch, preserving C short-circuit evaluation for double/float operands.
- Tests: 7 new tests added across `floating_point/`, `varargs/`, `functions/`, and `pointers/`. Full suite 1217 valid + 260 invalid = 1477 pass.
- Notes: CC99-001 and CC99-002 share the same root cause (Phase 1 using `rax` instead of `xmm0` for stack-overflow FP args), so fixing one fixes both.
