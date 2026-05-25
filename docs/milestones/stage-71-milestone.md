# Milestone Summary

## Stage 71: Block-Scope Static Variables - Complete

Stage 71 ships block-scope `static` variable support, enabling local static variables that persist their values across function calls and are stored in .bss or .data with RIP-relative addressing.

- Tokenizer: No changes (static keyword already recognized).
- Grammar/Parser: Modified `parse_statement` to accept `TOKEN_STATIC` at block scope; validates constant initializer and registers the variable with `SC_STATIC` storage class.
- AST/Semantics: Added `is_static` field and `static_label[256]` to `LocalVar` struct; added `LocalStaticVar` struct to track block-scope statics; scope/shadowing validation via existing `codegen_find_var` and scope tracking.
- Codegen: Updated all variable access paths (load, store, address-of, prefix/postfix inc/dec, subscript, member access) to emit RIP-relative [rel Lstatic_func_N] addressing for static locals. Added `codegen_emit_local_statics()` to emit static declarations to .bss (uninitialized) or .data (initialized) after string pool.
- Tests: 5 new tests (4 valid: test_block_static_persist, test_block_static_init, test_block_static_separate_funcs, test_block_static_nested; 1 invalid: test_block_static_nonconstant). Full suite 1148/1148 pass (709 valid, 213 invalid, 67 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- Docs: Updated README.md (progress statement, declarations note, not-yet-supported list, test totals) and grammar.md (storage class specifier restrictions).
- Notes: Label format `Lstatic_<func>_<N>` ensures unique global symbols (NASM local labels would cause linker errors when defined in .bss and referenced from code). Only constant initializers supported; arrays and structs out of scope.
