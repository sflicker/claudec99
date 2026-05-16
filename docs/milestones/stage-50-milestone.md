# Milestone Summary

## Stage 50: Object-like Macros - Complete

stage-50 ships object-like `#define` macro support with macro definition parsing, storage, and identifier-level expansion in ordinary source text, plus cross-file macro visibility.

- Preprocessor: Added `MacroTable` (dynamic array of `MacroDef` name/replacement pairs) shared across all preprocessing calls. `#define NAME replacement` directive parsed and stored. Identifier expansion in ordinary source text replaces macro names with replacement text (no re-scanning). Compatible redefinitions silently accepted; incompatible ones produce fatal error.
- Tests: 7 new valid tests (single-file and multi-file macro usage, expressions, compatible redefinition, statement macros), 1 new invalid test (incompatible redefinition), 1 new integration test. Two obsolete Stage 48 tests deleted (both directives now supported).
- Docs: `include/preprocessor.h` doc comment updated. Grammar section on preprocessing and macro expansion added to `docs/grammar.md`.
- Final result: All 907 tests pass (542 valid, 181 invalid, 25 integration, 39 print-AST, 99 print-tokens, 21 print-asm). No regressions.
- Notes: Macros defined in included headers are visible to the including file due to shared `MacroTable`. Function-like macros remain out of scope for future stages.
