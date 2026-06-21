# Milestone Summary

## Stage 163: Non-Constant Initializer for Global - Complete

stage-163 fixes a bug where `T *p = NULL;` at file scope was incorrectly rejected with "non-constant initializer for global" when `NULL` is defined (per GCC system stddef.h) as `((void *)0)`. The compiler now correctly recognizes null pointer constant casts (`AST_CAST` from `TYPE_POINTER` to `TYPE_INT` literal 0) as valid global pointer initializers, matching C99 semantics.

- Tokenizer: No changes.
- Grammar/Parser: Extended pointer-global initializer validator in `src/parser.c` (~line 4639) to accept `AST_CAST` nodes where `decl_type == TYPE_POINTER` and the child is `AST_INT_LITERAL("0")`.
- AST/Semantics: No AST node types added; parser now recognizes cast-wrapped null pointer constants in global declarations.
- Codegen: Added new branch in `codegen_add_global` for `TYPE_POINTER` globals initialized with an `AST_CAST` null pointer constant — sets `init_value = 0`, `is_initialized = 1`, emits `dq 0` in the data section.
- Tests: 1 new integration test (test_null_cast_global). Full suite: 2066 portable (165 unit, 1286 valid, 261 invalid, 183 integration, 50 print-AST, 100 print-tokens, 21 print-asm) pass. System-include: 183 pass. Optional-library: 2 pass.
- Docs: version.c updated to 01630000; self-host C0→C1→C2 verified with all tests passing.
- Notes: Enables proper compilation of programs using system headers where `NULL` is a cast expression rather than a bare integer literal.
