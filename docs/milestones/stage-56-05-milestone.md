# Milestone Summary

## Stage 56-05: Standard Predefined Macros - Complete

stage-56-05 ships three standard predefined macros (`__STDC__`, `__STDC_VERSION__`, and `__CLAUDEC99__`) that are available before preprocessing begins and work in `#if`, `#ifdef`, and normal source macro expansion.

- **Preprocessor**: Added three predefined macros to the macro table at the start of preprocessing in `src/preprocessor.c`: `__STDC__` (expands to 1), `__STDC_VERSION__` (expands to 199901), and `__CLAUDEC99__` (expands to 1). These behave as ordinary object-like macros; compatible redefinitions via `-D` or source are allowed; incompatible redefinitions are rejected; `#undef` works on all three.
- **Grammar/Parser**: No changes; predefined macros are a preprocessor-only feature.
- **AST/Semantics**: No changes.
- **Codegen**: No changes.
- **Tests**: Three new valid tests (`test_pp_predef_stdc_version_if__42.c`, `test_pp_predef_claudec99_ifdef__42.c`, `test_pp_predef_stdc_undef__1.c`) and one new invalid test (`test_pp_predef_redefine_incompatible__incompatible_replacement.c`). Full suite: 1014/1014 pass (628 valid, 196 invalid, 31 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- **Docs**: README.md updated to describe the three predefined macros and their availability in conditionals and expansions.
- **Notes**: Spec typo corrected (`___STDC__` with 3 underscores fixed to `__STDC__`); implementation bug fixed (hardcoded string length 15 for 16-character `__STDC_VERSION__` changed to `strlen()` calls).
