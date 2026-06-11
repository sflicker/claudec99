# Milestone Summary

## Stage 101: Block-Scope Static Arrays and Structs - Complete

Stage 101 lifts the codegen guard that rejected `TYPE_ARRAY`, `TYPE_STRUCT`, and `TYPE_UNION` block-scope static local variables, enabling patterns like `static int counts[8]` and `static struct Point p = {1, 2}` to persist across function calls with proper RIP-relative addressing.

- **Tokenizer**: No changes.
- **Grammar/Parser**: No changes.
- **AST/Semantics**: Added `ASTNode *init_node` field to `LocalStaticVar` struct to carry brace-list and string-literal initializers for aggregate types.
- **Codegen**: Removed guard in `SC_STATIC` arm; added array/struct static registration blocks in `codegen_statement`; extended `codegen_emit_local_statics` to emit initialized and uninitialized aggregate statics to `.data` and `.bss`; added `is_static` branches in array decay, subscript addressing, and struct member addressing.
- **Tests**: 6 new valid tests (uninitialized/initialized arrays, uninitialized/initialized structs, char arrays from string literals, counter persistence); 2 new invalid tests (non-constant initializers). All 1552 tests pass (880 valid, 250 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- **Docs**: Updated README.md to reflect stage 101 completion and aggregate static local support; docs/grammar.md unchanged (no grammar changes).
- **Notes**: Self-host C0→C1→C2 cycle passes cleanly with no compiler source changes needed.
