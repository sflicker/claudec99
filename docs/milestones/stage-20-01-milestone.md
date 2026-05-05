# Milestone Summary

## Stage 20-01: Declarator Refactor - Complete

Stage 20-01 refactors the parser's type and declaration handling to separate type specifiers from declarators, creating a proper declarator grammar that binds pointers, array syntax, and identifiers at parse time instead of in the type representation.

- **Tokenizer**: No changes.
- **Grammar/Parser**: Introduced `parse_type_specifier()`, `parse_type_name()`, and `parse_declarator()` as new internal parser functions. Updated grammar rules for `<declaration>`, `<type_specifier>`, `<type_name>`, `<declarator>`, `<direct_declarator>`, `<init_declarator>`, `<initializer_expression>`, `<function_declarator>`, `<parameter_declarator>`, `<function>`, `<parameter_declaration>`, `<cast_expression>`, and `<unary_expression>`. The refactor cleanly separates base type parsing from declarator syntax, enabling future support for comma-separated declarations.
- **AST/Semantics**: No AST changes. Semantic types (declaration types, parameter types, function return types) are preserved as-is. Internal `ParsedDeclarator` helper struct used during parsing but never appears in final AST. Pointer casts like `(int*)x` now parse correctly.
- **Codegen**: No changes. Codegen operates on final semantic types exactly as before.
- **Tests**: No new tests added; all 376 valid and 102 invalid tests continue to pass. Full suite: 478/478 pass.
- **Docs**: Grammar rules in `docs/grammar.md` updated to reflect the new declarator structure.
- **Notes**: This is a pure internal refactor with no change to user-visible language features or test coverage. The refactored grammar lays groundwork for future comma-separated declarations and maintains backward compatibility with all existing code.
