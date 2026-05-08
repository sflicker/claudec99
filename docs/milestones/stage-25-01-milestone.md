# Milestone Summary

## Stage 25-01: Function Pointer Declarations - Complete

stage-25-01 ships support for declaring function-pointer typed variables, parameters, file-scope objects, static objects, and extern declarations, with full type compatibility checking across redeclarations.

- Tokenizer: No changes.
- Grammar/Parser: Extended `declarator` rule to recognize `(*name)(params)` syntax for function pointers; added `func_ptr_param_list` rule for simplified parameter lists; updated parameter declaration, local declaration, and external declaration paths to build function-pointer types correctly.
- AST/Semantics: Added `TYPE_FUNCTION` to TypeKind enum; added `param_count` and `params[16]` fields to Type struct; implemented `type_function()` constructor; added `func_ptr_types_equal()` for deep comparison across redeclarations.
- Codegen: Added `TYPE_FUNCTION` case to `type_kind_bytes()` (returns 0; function types are never directly allocated — always wrapped in TYPE_POINTER).
- Tests: 12 new tests (7 valid, 5 invalid). Valid suite: function pointer locals, named parameters, file-scope, static, extern-with-definition, parameters, return-pointer. Invalid suite: param type mismatch, return type mismatch, duplicate static, call-through, assign-to-function-pointer. Full suite 688/688 pass.
- Docs: Updated grammar.md with `func_ptr_declarator` and `func_ptr_param_list` rules; added restriction notes on pointer-to-function-pointer and function-returning-function-pointer (out of scope); updated README.md with "Through stage 25-01" and test totals.
- Notes: No assignment-to or call-through function pointers (those remain out of scope). Pointer-to-function-pointer (`**fp`) and function-returning-function-pointer also remain out of scope.
