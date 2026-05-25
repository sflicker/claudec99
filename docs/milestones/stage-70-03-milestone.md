# Milestone Summary

## Stage 70.03 – Add Line/Column to Errors and Warnings - Complete

stage-70-03 ships parser error messages with `<file>:<line>:<col>:` position prefixes using Token source location data from stage 70.02.

- Tokenizer: No changes.
- Grammar/Parser: Added `PARSER_ERROR(parser, ...)` macro in parser.c; refactored all 107 error call sites to use `compile_error_at` with Token position info.
- AST/Semantics: No changes.
- Codegen: Updated `codegen_warn_const_discard` to use `compile_warning_at(NULL, 0, 0, ...)` and replaced local `warnings_are_errors` logic with global `g_warnings_are_errors`.
- Error/Warning APIs: Added `compile_error_at(file, line, col, ...)` and `compile_warning_at(file, line, col, ...)` functions; refactored `compile_error` to use internal `do_compile_error` helper; added global `g_warnings_are_errors` controlled by `-Werror` flag.
- Tests: All 1143 tests pass (705 valid, 212 invalid, 67 integration, 39 print-AST, 99 print-tokens, 21 print-asm). Invalid tests use substring matching to tolerate position prefixes.
- Docs: Updated "Through stage..." line in README from 70.02 to 70.03.
- Notes: Codegen errors (non-parser) and codegen warnings still print without position prefixes; AST nodes do not carry token information. This is a known limitation for future stages.
