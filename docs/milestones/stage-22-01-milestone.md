# Milestone Summary

## Stage-22-01: File-scope Uninitialized Object Declarations - Complete

stage-22-01 ships file-scope (global) uninitialized object declarations with zero-initialization in the .bss section and RIP-relative addressing.

- **Parser**: Fixed `parse_external_declaration` to handle array declarations (`is_array`) at file scope, properly setting `TYPE_ARRAY` and `full_type` for global array declarations.
- **Codegen**: Added global variable infrastructure (GlobalVar struct, `codegen_add_global`, `codegen_find_global`, global-aware emitters for load/store/addressing). Implemented `.bss` section emission with `resb/resw/resd/resq` directives. Updated expression evaluation to fall back to global lookups when no local variable matches. Modified `codegen_translation_unit` to register globals before function code and emit `.bss` before `.text`.
- **Docs**: Updated `docs/grammar.md` to clarify that file-scope object declarations must be uninitialized (not a blanket ban).
- **Tests**: Added 9 valid tests (basic, shadowing, persistence, type coverage, pointers, arrays, zero-init) and 2 print_asm tests (BSS layout, RIP-relative addressing). Added 1 invalid test for duplicate global declarations. Final test suite: **645 passed** (401 valid, 111 invalid, 21 print_asm, 66 print_ast, 46 print_tokens).
- **Notes**: Duplicate global check added for correctness (not in spec). Local lookup before global fallback ensures natural shadowing. Spec has minor typo (`arr; resd 10` should be `arr: resd 10`).
