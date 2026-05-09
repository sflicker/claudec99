# Milestone Summary

## Stage 27: Declaration Specifier Refactor - Complete

stage-27 refactors declaration specifier parsing to accept a list of specifiers rather than a fixed optional storage class plus single type, enabling better duplicate detection and more flexible future extensions.

- Tokenizer: No changes.
- Grammar/Parser: Updated `<declaration_specifiers>` grammar from `[ <storage_class_specifier> ] <type_specifier>` to `<declaration_specifier> { <declaration_specifier> }`. Added `parse_declaration_specifiers()` helper function to parse a list of specifiers and detect duplicate storage classes and type specifiers with clear error messages. Updated `parse_external_declaration()` to use the new helper.
- AST/Semantics: No changes.
- Codegen: No changes.
- Tests: Added 4 new invalid test cases covering duplicate storage classes and duplicate type specifiers. Full suite 573/573 pass (436 valid + 137 invalid).
- Docs: Updated `docs/grammar.md` to reflect the new production rules for declaration specifiers.
- Notes: This is an internal refactor with no user-visible feature changes. All existing tests continue to pass.
