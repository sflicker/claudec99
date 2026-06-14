# Milestone Summary

## Stage 119: FP Compound Assignment on Struct Members — Complete

stage-119 ships codegen fixes for floating-point compound assignment on file-scope (global) struct fields, addressing six bugs in type-resolution functions.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: Six fixes in `src/codegen.c`:
  1. `expr_result_type()` AST_MEMBER_ACCESS VAR_REF base: added global struct fallback via `codegen_find_global` (Bug 1).
  2. `expr_result_type()` AST_MEMBER_ACCESS DEREF base: added global pointer-to-struct fallback (Bug 2).
  3. `expr_result_type()` AST_ARROW_ACCESS: added global pointer-to-struct fallback (Bug 3).
  4. `sizeof_type_of_expr()` AST_MEMBER_ACCESS VAR_REF base: added global struct fallback (Bug 4).
  5. `sizeof_type_of_expr()` AST_ARROW_ACCESS: added global pointer-to-struct fallback (Bug 5).
  6. `emit_arrow_addr()` VAR_REF base: added global pointer-to-struct fallback so `gp->field` works as an lvalue for global pointer variables (additional fix discovered during testing).
- Tests: 7 new valid tests added (all in `test/valid/structs/`): global struct FP compound assignment variants, global pointer-to-struct arrow access, local struct regression, mixed storage classes, and accumulator loop pattern. Full suite 1879/1879 pass (1195 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit).
- Docs: `src/version.c` bumped to stage "01190000"; `docs/self-compilation-report.md` updated with stage-119 self-host result.
- Notes: Root cause was silent skip of FP arithmetic path when both operands were global struct FP fields, leaving integer instructions operating on IEEE 754 bit patterns. Self-host C0→C1→C2 passed with no source changes during bootstrap.
