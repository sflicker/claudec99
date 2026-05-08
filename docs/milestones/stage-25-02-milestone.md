# Milestone Summary

## stage 25-02: Assign Function Designators to Function Pointers - Complete

stage-25-02 ships assignment and initialization of function pointers from function designators, with full type compatibility validation.

- Tokenizer: No changes.
- Grammar/Parser: Parser now accepts optional `= identifier` initializer in file-scope function pointer declarations; validates that the identifier names a function and that its signature matches the declared pointer type.
- AST/Semantics: No AST structure changes; function designators are represented as `AST_VAR_REF` nodes in initializers.
- Codegen: Added `is_label_init` and `init_label` fields to `GlobalVar` for label-reference global initializers. Implemented `func_ptr_types_equal_cg` helper for deep function type comparison. `AST_VAR_REF` handler now detects function names and emits `lea rax, [rel name]` with function pointer result type. `AST_ASSIGNMENT` handler validates function pointer LHS against RHS function type. `codegen_emit_data` emits `dq label_name` for label initializers.
- Tests: 4 tests added (2 valid: local and file-scope assignment; 2 invalid: incompatible return type and parameter type). 1 invalid test removed (became valid). Full suite 691 passed.
- Docs: grammar.md note updated (out-of-scope removed); README.md updated to stage 25-02 and clarified function pointer feature scope.
- Notes: Calling through function pointers remains out of scope. No changes to tokenizer, AST node types, or grammar productions.
