# Milestone Summary

## Stage 146: Strength Reduction: Power-of-Two Multiply/Divide - Complete

stage-146 adds strength reduction to the stage-142 optimizer: multiplication by a compile-time power-of-two constant is rewritten to a left shift, and division by a power-of-two constant (for unsigned or statically non-negative signed dividends) is rewritten to a right shift.

- **Optimizer**: New strength-reduction block in `src/optimize.c` within `optimize_expr`, firing after stage-145 algebraic identity rules and only under `-O1`. Three rules: `x * 2^N → x << N`, `2^N * x → x << N` (commutative), and `x / 2^N → x >> N` (for unsigned or statically non-negative constant dividends). All three mutate the AST_BINARY_OP node in place.
- **Power-of-two detection**: Uses bitwise test `(rval > 1L) && ((rval & (rval - 1L)) == 0L)`; shift amount computed via iterative right-shift loop.
- **Division scope**: `x / 2^N` only fires when `left->is_unsigned` is true or when the dividend is a non-negative integer literal (set by codegen and parser respectively). Signed variables with unknown sign at compile time are correctly skipped.
- **Tests**: Five new integration tests: multiply by power-of-two (two forms: right-operand and commutative), divide by power-of-two (unsigned only), no reduction for signed division, and composition with constant folding. All use `-O1`.
- **Results**: All 2003 portable tests pass (165 unit, 1286 valid, 261 invalid, 120 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- **Self-host**: C0→C1→C2 verified with all tests passing at every stage. No source changes needed during bootstrap.
- **Notes**: Stage 145's multiplicative identity rule (`x*1→x`) already handles `* 2^0`; this stage only fires for N ≥ 1. Memory management carefully avoids double-free by nulling child pointers before `ast_free`.
