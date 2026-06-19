# Milestone Summary

## Stage 139: Preprocessor `#if` expression gaps - Complete

stage-139 fixes three preprocessor `#if`/`#elif` expression evaluator gaps that blocked the compiler from parsing system headers (glibc, features.h).

- **Preprocessor (PP-01)**: Integer literal suffixes (`u/U/l/L`) and hexadecimal literals (`0x`/`0X` prefix) now parsed in `eval_cond_primary`. Fixes expressions like `__STDC_VERSION__ > 201710L`.
- **Preprocessor (PP-02)**: `#if` and `#elif` directives now pre-expand macros in the condition text before evaluation, per C99 §6.10.1. Function-like macros in conditions are now supported. Special protection added to `expand_macros_text()` to preserve `defined(X)` / `defined X` from expansion.
- **Preprocessor (PP-03)**: Ternary operator `?:` added to condition expression evaluator with correct precedence (lower than `||`, right-associative).
- **Codegen**: Version bumped to 13900000.
- **Tests**: 9 new integration tests added. `run_tests_sysinclude.sh` helper script added for manual system-header validation. Full suite: 1979 tests pass (1284 valid, 262 invalid, 97 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- **Verification**: Self-host C0→C1→C2 cycle passes cleanly with all tests passing at every stage, no source changes needed during bootstrap.
- **Notes**: Pre-expansion initially broke `defined(X)` tests (11 regressions) until special case added to protect `defined` operators. Spec test `test_pp_if_funclike_token_paste` had a naming inconsistency (`CAT_VAL_1` vs `CAT_VAL1`) that was corrected.
