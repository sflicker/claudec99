# Milestone Summary

## stage-83: Project Source Converted to Strict ISO C99 - Complete

stage-83 converts the compiler's own source to strict ISO C99 (passes `gcc/clang -std=c99 -pedantic-errors` on every module). This is a source-conformance and build-configuration change, not a language feature; no compiler behavior changed.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: No changes.
- Build: Added `util_strdup()` helper in util module (replacing non-standard `strdup`); replaced GNU `__attribute__` with portable CC_NORETURN/CC_PRINTF macros; removed redundant `ASTNode` typedef from codegen.h; CMakeLists.txt locked to C99 (`CMAKE_C_STANDARD 99`, `CMAKE_C_EXTENSIONS OFF`, `-Wall -Wextra -pedantic`).
- Tests: No new tests added (behavior-preserving refactor). Full suite 1259/1259 pass (787 valid, 235 invalid, 73 integration, 43 print-AST, 100 print-tokens, 21 print-asm).
- Docs: README.md updated to reflect C99 implementation language and stage progress.
- Notes: None.
