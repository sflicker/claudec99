# Milestone Summary

## Stage 138: auto and register Storage-Class Specifiers - Complete

stage-138 ships support for the `auto` and `register` C99 storage-class specifiers (CC99-011 and CC99-012).

- Tokenizer: Added `TOKEN_AUTO` and `TOKEN_REGISTER` keywords with keyword recognition and display names.
- Grammar/Parser: `parse_declaration_specifiers` rejects both at file scope; `parse_statement` handles `auto` (treated as default automatic storage) and `register` (marks declaration `SC_REGISTER`) at block scope; `parse_parameter_declaration` accepts `register` as a leading qualifier.
- AST/Semantics: `SC_AUTO=8` and `SC_REGISTER=16` added to `StorageClass` enum in `ast.h`; `ast_pretty_printer` displays both.
- Codegen: `is_register` field added to `LocalVar`; set for all `SC_REGISTER` declarations (struct, array, FP, scalar paths); `AST_ADDR_OF` rejects address-of on register variables with a compile error.
- Tests: 5 new tests (2 valid returning 27, 3 invalid). Full suite 1970/1970 pass.
- Docs: Self-compilation report updated; kickoff artifact generated.
- Notes: Both specifiers are only for local (block-scope) use; `register` generates identical code to automatic but enforces no address-of at compile time.
