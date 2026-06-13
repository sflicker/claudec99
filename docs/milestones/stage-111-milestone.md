# Milestone Summary

## Stage 111 — float/double Comparisons and Boolean Contexts: Complete

Stage 111 ships floating-point comparisons and boolean contexts: comparison operators (`<`, `<=`, `>`, `>=`, `==`, `!=`) on float/double operands, FP values in control-flow conditions (`if`, `while`, `for`, `?:`), logical NOT on FP operands, and mixed int/FP comparisons with automatic type promotion.

- Tokenizer: No changes (comparison tokens already exist).
- Grammar/Parser: No changes (comparison and conditional expressions use existing rules).
- AST/Semantics: Extended `emit_cond_cmp_zero()` to detect FP condition type and call new `emit_fp_bool_to_rax()` helper; same extended in `AST_CONDITIONAL_EXPR` condition handling.
- Codegen: Added `emit_fp_bool_to_rax()` helper (convert FP in xmm0 to 0/1 in rax via ucomisd/ucomiss and flag-based assembly); FP comparison block in `AST_BINARY_OP` (save/restore left operand, emit comparison instruction, set flag bits and movzx); logical NOT on FP via flag-based assembly with NaN-correct semantics (`sete + setnp + and` for `!`).
- Tests: 8 new valid tests (FP comparisons, boolean contexts, mixed int/FP, ternary conditions, loops). Full suite 1643 passed, 0 failed, 1643 total.
- Docs: README updated; floating-point-plan.md Stage 111 section marked complete with implementation notes; checklist updated.
- Notes: FP boolean context treats NaN as true (one valid C99 interpretation). Implementation uses `sete+setnp+and` pattern from spec for logical NOT (not the plan's "xor al, 1" variant) — same semantics but cleaner. Helper function forward-declaration issue: `emit_fp_bool_to_rax` was initially placed at function-end, causing C99 implicit-declaration error since it's called inside `codegen_expression`; moved to FP helper section. Self-host C0→C1→C2 passes cleanly with no source changes needed during bootstrap.
