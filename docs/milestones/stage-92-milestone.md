# Milestone Summary

## stage-92: Self-Compile Validation - Incomplete

stage-92 began self-compilation validation by building and testing the compiler with its own code generator. The bootstrap surfaced and fixed seven real defects and capacity limits but halted at a struct-by-value limitation in the lexer, preventing full self-hosting.

- Tokenizer: No changes.
- Grammar/Parser: Parser `for`-statement builder updated to use `ast_add_child` (required by dynamic AST children array).
- AST/Semantics: `ASTNode.children` converted from fixed-size array (hard cap 64) to dynamically allocated, doubling array. Fixes silent truncation of large translation units, blocks, and switches (e.g., dropping `main` past the 64th declaration).
- Codegen: `codegen_emit_externs` suppresses duplicate/redundant `extern` directives for objects both declared and defined in the same translation unit (fixes NASM redefinition errors). Hoisted six block-scope `static const char *[]` arrays to file scope (block-scope static arrays not yet supported).
- Constants: `MAX_STRING_LITERALS` 256 → 2048; `MAX_SWITCH_LABELS` 64 → 256 (codegen and token_type_name switches needed larger limits).
- Stdlib: Added `strtol` and `strtoul` declarations to stub `stdlib.h`.
- Version: Changed `main` signature to `char **argv` form (fixes `T *name[]` element mis-typing). Bumped VERSION_STAGE to "00920000" (MINOR version intentionally kept at "01").
- Tests: All 1302 tests pass (819 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). No regressions.
- Docs: `docs/self-compilation-report.md` rewritten with accurate bootstrap findings. README updated (limits tables, stub-header list, through-stage note — no self-compilation claim). Grammar unchanged (no grammar impact).
- Notes: **Full self-hosting is NOT yet achieved.** The bootstrap halts in `lexer.c` at struct-by-value parameters/returns (Token interface), a documented unsupported feature. Seven fixes were committed; the remaining blocker (struct-by-value) is deferred to a dedicated future stage. No claims of self-compilation are made.
