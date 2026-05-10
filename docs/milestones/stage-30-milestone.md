# Milestone Summary

## Stage 30: Struct Definition, Layout, and sizeof - Complete

stage-30 ships struct definitions with natural-alignment field layout and sizeof support.

- Tokenizer: Added TOKEN_STRUCT to lex the `struct` keyword.
- Grammar/Parser: Added struct_specifier rule with struct_declaration_list and struct_declarator_list; parse_struct_specifier() computes field offsets using natural alignment (each field padded to its own alignment, struct size rounded to max field alignment).
- AST/Semantics: Added TYPE_STRUCT to TypeKind; type_struct() constructor creates struct types; struct tag table (PARSER_MAX_STRUCT_TAGS=32) tracks named structs in the parser.
- Codegen: Extended sizeof to handle struct types (both AST_SIZEOF_TYPE and AST_SIZEOF_EXPR); updated local variable allocation and compute_decl_bytes to track struct sizes.
- Tests: 3 new tests for struct sizeof (Point, Mixed, and variable instances). Full suite 739 passed, 0 failed (3 new tests, no regressions).
- Docs: Updated docs/grammar.md with struct_specifier, struct_declaration_list, struct_declaration, struct_declarator_list rules.
- Notes: Only named structs supported; member access is out of scope for this stage. Three spec typos were corrected during implementation (grammar missing ">", tests missing ";" and "}").
