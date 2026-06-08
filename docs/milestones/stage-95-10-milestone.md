# Milestone Summary

## Stage 95-10: Remove static char arrays from parser.h - Complete

Stage 95-10 converts seven `char field[MAX_NAME_LEN]` embedded arrays in parser.h structs to `const char *` pointers, eliminating a major source of fixed-capacity overflow risk in the parser symbol tables.

- **Tokenizer**: No changes.
- **Parser**: Converted `EnumConst.name`, `EnumTag.tag`, `StructTag.tag`, `UnionTag.tag`, `TypedefEntry.name`, `FuncSig.name`, and `GlobalObjSig.name` from `char[MAX_NAME_LEN]` to `const char *` (7 fields total). All `strncpy` calls replaced with direct pointer assignments. Three local `char[256]` temporary buffers simplified to `const char *` pointers into lexer-owned storage (struct specifier, union specifier, enum specifier tag handling, enum constant inline naming).
- **AST/Semantics**: No changes.
- **Codegen**: No changes.
- **Tests**: All 1479 tests pass (165 unit, 828 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). Verified with C0→C1→C2 self-hosting cycle; all stages pass full test suite.
- **Docs**: Updated `docs/fixed-capacity-inventory.md` to mark parser.h struct name/tag fields completed in stage 95-10. Updated `src/version.c` (stage 00951000).
- **Notes**: All name and tag values derive from `parser->current.value` (persistent pointer into lexer string pool since stage 95-08), so direct pointer assignment is safe. The `MAX_NAME_LEN` limit no longer applies to parser.h struct fields.
