# Milestone Summary

## Stage 118: Pointer Relational Operators — Complete

stage-118 ships support for the four pointer relational comparison operators (`<`, `<=`, `>`, `>=`) when applied to pointer operands.

- **Tokenizer**: No changes.
- **Grammar/Parser**: No changes; the grammar already includes these operators.
- **AST/Semantics**: No changes.
- **Codegen**: Extended `is_pointer_cmp` in `codegen_expression()` AST_BINARY_OP to cover all six comparison operators (was `==`/`!=` only); added validation guard `is_relcmp` that rejects `pointer < integer` before the null-pointer-constant check (only `==`/`!=` against null are valid per C99); added `|| is_pointer_cmp` to setcc selection so pointer relational comparisons emit unsigned `setb`/`setbe`/`seta`/`setae` instead of signed `setl`/`setle`/`setg`/`setge`.
- **Tests**: 9 new tests (7 valid pointer comparison patterns, 2 invalid rejections). All 1872 tests pass (1188 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- **Self-host**: C0→C1→C2 bootstrap passes cleanly; all 1872 tests pass at each stage. No source changes needed during bootstrap. Compiler's own pointer comparison loops now use unsigned setcc variants.
- **Notes**: Pointer addresses are unsigned 64-bit values, so all pointer relational comparisons route through the unsigned code path. The compiler's own source contains pointer loops like `p < end` which now emit the correct unsigned comparison instructions.
