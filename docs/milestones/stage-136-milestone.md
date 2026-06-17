# Milestone Summary

## Stage 136: sizeof of Pointer-Arithmetic Expressions - Complete

Stage 136 fixes a bug in `sizeof_type_of_expr` where `sizeof` applied to pointer-arithmetic expressions returns the element size instead of the pointer/ptrdiff_t size.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: Fixed two bugs in `sizeof_type_of_expr` in `src/codegen.c`: (1) added a pointer/array guard in the `AST_BINARY_OP` case for `+`/`-` operators, returning `TYPE_POINTER` (size 8 on LP64) when either operand is a pointer or array type, and (2) added an `AST_STRING_LITERAL` case returning `TYPE_POINTER` so string literals in binary expressions are treated as pointers. Version bumped to 13600000.
- Tests: 10 new tests in `test/valid/expressions/` (8 new pointer-arithmetic sizeof cases + 2 regression guards). Full suite: 1961/1961 pass (1277 valid, 259 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- Docs: Checklist updated (Lvalue conversion item marked done). Kickoff and self-compilation notes generated.
- Notes: Self-host C0→C1→C2 completed with no source changes needed during bootstrap.
