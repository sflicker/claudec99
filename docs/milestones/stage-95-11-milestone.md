# Milestone Summary

## Stage 95-11: Remove static char arrays from codegen.h - Complete

Stage 95-11 converts the remaining fixed-capacity `char[MAX_NAME_LEN]` name/label buffers in codegen.h structs to `const char *` pointers, and replaces the 2D `char user_labels[MAX_USER_LABELS][MAX_NAME_LEN]` array with a `Vec` of pointers.

- **Tokenizer**: No changes.
- **Parser**: No changes.
- **AST/Semantics**: No changes.
- **Codegen**: Converted `LocalVar.name`, `LocalVar.static_label`, `LocalStaticVar.label`, `GlobalVar.name`, and `GlobalVar.init_label` from `char[MAX_NAME_LEN]` to `const char *` (5 fields total). Names from AST/lexer-owned storage use direct pointer assignment; generated labels (`Lstatic_*` and `Lstr*`) use `util_strdup`. Replaced 2D array `CodeGen.user_labels[MAX_USER_LABELS][MAX_NAME_LEN]` plus `user_label_count` field with `Vec user_labels; /* const char * */`, eliminating the 64-label-per-function cap entirely. Removed `MAX_USER_LABELS` from `include/constants.h`.
- **Tests**: All 1479 tests pass (165 unit, 828 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). Verified with C0→C1→C2 self-hosting cycle; all stages pass full test suite.
- **Docs**: Updated `docs/fixed-capacity-inventory.md` to mark `MAX_USER_LABELS` DONE and codegen struct fields completed in stage 95-11. Updated `src/version.c` (stage 00951100). Updated README.md to reflect stage 95-11 scope and remaining `MAX_NAME_LEN` application (`StructField.name` in type.h).
- **Notes**: The only remaining `MAX_NAME_LEN` application is `StructField.name` in `type.h`. All codegen symbol table buffers are now pointer-based with dynamic allocation via `util_strdup` for generated labels.
