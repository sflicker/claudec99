# Milestone Summary

## Stage 26 General Declarator Cleanup: Complete

Stage 26 ships general declarator refactoring that unifies function pointer parsing with ordinary declarators, eliminating the special-case function-pointer grammar path.

- Tokenizer: No changes.
- Grammar/Parser: Refactored declarator parsing to handle function pointers through the same recursive machinery as identifiers, pointers, arrays, and functions. Removed `<func_ptr_declarator>`, `<func_ptr_param_list>`, and `<func_ptr_param>` rules. Added `<parameter_declarator>` with optional declarator to allow unnamed parameters in prototypes and function pointer signatures.
- AST/Semantics: Extended `ParsedDeclarator` with `fp_param_types[]` and `fp_param_count` fields for inline function pointer type capture. Replaced separate `parse_func_ptr_param_types` and `build_func_ptr_type` functions with small inline `build_fp_type` helper.
- Codegen: No changes.
- Tests: 3 new tests added (2 valid, 1 invalid); existing tests adjusted for error message update. Full suite 702/702 pass (434 valid, 132 invalid, 24 print-AST, 88 print-tokens, 21 print-asm).
- Docs: Grammar updated to reflect recursive declarator syntax and parameter declarator rule.
- Notes: Function definitions still require named parameters; error message for `int (*fp())(int)` updated to "functions returning function pointers are not supported."
