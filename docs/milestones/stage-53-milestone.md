# Milestone Summary

## Stage 53: Predefined Macros - Complete

stage-53 ships support for standard predefined macros `__FILE__` (expands to a string literal of the current source file) and `__LINE__` (expands to an integer literal of the current source line number).

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: No changes.
- Preprocessor: Added `current_line` tracking in `preprocess_internal`. Identifier expansion checks for `__FILE__` (emits source path as quoted string with proper escape handling) and `__LINE__` (emits decimal line number) before macro-table lookup.
- Tests: 3 new tests added (2 unit, 1 integration). Full suite 943/943 pass (564 valid, 193 invalid, 27 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- Docs: README updated with stage reference and feature description.
- Notes: Predefined macros are recognized by exact name before user-defined macros; no special handling of macro redefinition or scoping required (they are keywords in the macro namespace).
