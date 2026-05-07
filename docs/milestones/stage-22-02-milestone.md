# Milestone Summary

## Stage 22-02: Global Initializers and File-Scope Declaration Validation - Complete

stage-22-02 ships support for initialized file-scope global variable declarations with constant integer expressions and tightens validation around conflicting file-scope declarations.

- Tokenizer: No changes.
- Grammar/Parser: Added `GlobalObjSig` struct and global registry to track file-scope objects and functions; `parse_external_declaration` now handles optional constant initializers and comma-separated declarators at file scope; conflict detection for duplicate objects, incompatible redeclarations, and function/object name collisions at parse time.
- AST/Semantics: No structural changes; validation moved to parser.
- Codegen: Added `is_initialized` and `init_value` fields to `GlobalVar`; `codegen_add_global` extracts and stores initializer values; added `data_init_directive()` helper and `codegen_emit_data()` for `.data` section output; modified `codegen_emit_bss()` to skip initialized globals; `codegen_translation_unit` emits `.data` section before `.bss`.
- Tests: 12 new tests (7 valid, 5 invalid). Full suite: 657 passed (408 valid, 116 invalid, 66 print-AST, 46 print-tokens, 21 print-asm).
- Docs: grammar.md updated to reflect constant initializer requirements and lifted restriction on initialized globals.
- Notes: Spec contained minor typos in test case bodies (trailing operator, missing variable name); tests corrected per intent. Array and non-constant initializers remain out of scope.
