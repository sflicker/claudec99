# Milestone Summary

## Stage 143: Constant Integer Binary Folding - Complete

stage-143 implements the first real optimization rule in the stage-142 infrastructure: **constant integer binary folding**. When both children of an `AST_BINARY_OP` node are `AST_INT_LITERAL`, the optimizer computes the result at compile time and replaces the entire binary node with a single `AST_INT_LITERAL`. Unary bitwise-NOT (`~`) folding on constant operands is also implemented. All folding is gated behind `-O1`; the default `-O0` path is unaffected. No changes to other modules or the grammar.

- Optimizer rules: `src/optimize.c` extended with constant-folding logic in `optimize_expr()`: arithmetic (`+`, `-`, `*`, `/`, `%`) with div-by-zero guard, bitwise (`&`, `|`, `^`, `<<`, `>>`), relational (`<`, `<=`, `>`, `>=`, `==`, `!=`), logical short-circuit (`&&`, `||`), and unary bitwise-NOT (`~`).
- Result types: relational/logical produce `TYPE_INT` with value 0 or 1; arithmetic/bitwise inherit type from left operand.
- Includes: `#include <stdio.h>`, `#include <string.h>`, `#include "util.h"` added to `src/optimize.c`.
- Tests: 6 new integration tests with `-O1` flag: `test_fold_arithmetic`, `test_fold_bitwise`, `test_fold_relational`, `test_fold_logical`, `test_fold_divzero_skipped`, `test_fold_nested`.
- Test results: All 1988 portable tests pass (165 unit, 1286 valid, 261 invalid, 105 integration, 50 print-AST, 100 print-tokens, 21 print-asm); 6 new integration tests added.
- Self-host: C0→C1→C2 verified; all tests pass at every stage. C0: `00.03.01430000.01077`, C1: `00.03.01430000.01078`, C2: `00.03.01430000.01079`.
- Version: `src/version.c` bumped to `VERSION_STAGE "01430000"`.
