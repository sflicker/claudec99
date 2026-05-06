# Milestone Summary

## Stage 21-01: Function Declarator Refactoring - Complete

stage-21-01 refactors function declarations and definitions to use the general `<declarator>` machinery instead of dedicated `<function_declarator>` and `<parameter_declarator>` productions.

- Tokenizer: No changes.
- Grammar/Parser: Removed `<function_declarator>` and `<parameter_declarator>` productions. Extended `<direct_declarator>` to include function form `identifier ( [ parameter_list ] )`. Updated `<function>` and `<parameter_declaration>` to use `<declarator>`. Parser's `parse_declarator` now detects identifier(...) patterns and sets `is_function` flag on `ParsedDeclarator`. Validation in `parse_function_decl` ensures the declarator is a function declarator; error: "'f' is not a function declarator".
- AST/Semantics: No changes.
- Codegen: No changes.
- Tests: 1 new test (test_invalid_105__not_a_function_declarator.c). Full suite: 631 passed, 0 failed.
- Docs: grammar.md updated; README.md updated to reflect stage 21-01 and new test total.
- Notes: Parser-only refactoring unifying parameter and function declaration syntax under the general declarator framework.
