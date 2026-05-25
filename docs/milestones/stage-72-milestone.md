# Milestone Summary

## Stage 72: Named Union Support - Complete

stage-72 ships complete named union support with layout computation, member access, and nested type support.

- Tokenizer: Added `TOKEN_UNION` keyword recognition and token display.
- Grammar/Parser: Implemented `parse_union_specifier()` with union tag table, integrated `TYPE_UNION` into type specifier and declaration specifier paths, and extended sizeof to handle unions.
- AST/Semantics: Added `TYPE_UNION` to TypeKind; reused `StructField` for union member metadata with all offsets set to 0; union size = maximum member size.
- Codegen: Updated member access operators (`.` and `->`), sizeof, local/global declarations, whole-union assignment, and BSS emission to handle unions; first-member initialization via brace lists; fixed struct/union BSS allocation.
- Tests: 10 new tests (9 valid, 1 invalid) covering sizeof, member access, pointer access, nested types, whole assignment, and globals. Full suite: 1158/1158 pass.
- Docs: Added `<union_specifier>` to grammar; updated README.md stage reference and test totals.
- Notes: Spec ambiguity in "Union inside struct" test corrected (union name was `U`, not `Value`); incomplete union variable declarations properly rejected at codegen.
