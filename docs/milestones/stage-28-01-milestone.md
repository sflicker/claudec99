# Milestone Summary

## Stage 28-01: Simple Typedef Names - Complete

stage-28-01 ships simple typedef aliases for existing integer scalar types (char, short, int, long).

- **Tokenizer**: Added `TOKEN_TYPEDEF` keyword token and updated lexer to recognize `"typedef"`.
- **Grammar/Parser**: Extended declaration specifiers to parse `typedef` as a storage-class specifier. Added typedef registry with block-scope tracking via `scope_depth` counter. Parser recognizes typedef names as type specifiers when used in declarations, function signatures, casts, and `sizeof`. Implemented `parser_find_typedef()`, `parser_register_typedef()`, and `parser_leave_scope()` helpers to manage typedef scoping across nested blocks.
- **AST**: Added `SC_TYPEDEF` storage class and `AST_TYPEDEF_DECL` node type for typedef declarations.
- **Codegen**: Added no-op case for `AST_TYPEDEF_DECL` in code generator; typedefs do not emit runtime code.
- **Tests**: Added 6 valid tests (block scope, file scope, function signatures, sizeof) and 4 invalid tests (initializers, duplicates, undeclared identifiers, missing base type). Full suite: 716 passed, 0 failed.
- **Docs**: Updated `grammar.md` with storage_class_specifier and typedef_name productions.
- **Notes**: Spec contained four typos in test cases (corrected in implementation). The `!has_type` guard in declaration specifiers prevents typedef alias names from being consumed as second type specifiers.
