# Milestone Summary

## Stage 21-02: Separate Function Definitions from Declarations - Complete

Stage 21-02 refactors the external declaration parser to distinguish function definitions from function declarations, enabling separate grammar paths based on whether a function has a body.

- Tokenizer: No changes.
- Grammar/Parser: Removed `parse_function_decl` function. Replaced `parse_external_declaration` to parse the common type+declarator prefix, then dispatch: function with `{` → definition (AST_FUNCTION_DECL with body), function with `;` → prototype (AST_FUNCTION_DECL without body), non-function with `{` → error, function with `=` → new error "function declaration cannot have an initializer".
- AST/Semantics: No AST node type changes; function definitions and prototypes use the same AST_FUNCTION_DECL node structure.
- Codegen: No codegen changes.
- Tests: 2 new tests added (1 valid pointer-return prototype+definition, 1 invalid function initializer). Full suite 633/633 pass.
- Docs: Updated `docs/grammar.md` to reflect new `<external_declaration>` and `<function_definition>` rules. Updated README.md stage marker and test count.
- Notes: File-scope object declarations (non-function declarators followed by `;`) are parsed but not code-generated (out of scope for this stage).
