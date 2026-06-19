# stage-146 Transcript: Strength Reduction: Power-of-Two Multiply/Divide

## Summary

Stage 146 implements strength reduction optimizations in the stage-142 AST-level optimizer. When the optimizer encounters multiplication or division by a compile-time power-of-two constant, it rewrites the operation to a bitwise shift — a much faster operation on most CPU architectures.

Three transformation rules are implemented, all firing under `-O1` in the `optimize_expr` function after the stage-145 algebraic identity block:

1. **Multiply right operand**: `x * 2^N → x << N` when the right operand is a power-of-two literal.
2. **Multiply left operand (commutative)**: `2^N * x → x << N` when the left operand is a power-of-two literal. The operands are swapped and the left value is shifted.
3. **Divide by power-of-two**: `x / 2^N → x >> N` when the right operand is a power-of-two literal AND the dividend is either unsigned or a non-negative constant. This restriction exists because arithmetic right shift rounds toward negative infinity, whereas C signed division rounds toward zero.

All three rules mutate the AST_BINARY_OP node in place: the old power-of-two literal is freed, a new AST_INT_LITERAL holding the shift amount is installed, and the operator string is changed to shift. The node's result type is preserved from the original binary operation.

## Changes Made

### Step 1: Optimizer Block in src/optimize.c

- Located the end of `optimize_expr` function (after the stage-145 algebraic identity block and before the final `return node`).
- Inserted a new strength-reduction block guarded by the AST_BINARY_OP type check with exactly two children.
- Declared all local variables at the top of the outer `if` block for C0 bootstrap compatibility (no VLAs, no declarations after statements).
- Computed power-of-two test for both left and right operands using the bitwise mask `(val > 1L) && ((val & (val - 1L)) == 0L)`.
- Computed shift amounts via iterative right-shift loops (compile-time only, no runtime cost).
- Implemented three transformation rules with early returns on each match:
  - `x * 2^N`: freed right operand, installed shift-amount literal in `node->children[1]`, changed operator to `"<<"`.
  - `2^N * x`: moved right operand to left slot, freed original left operand, installed shift-amount literal in `node->children[1]`, changed operator to `"<<"`.
  - `x / 2^N`: freed right operand, installed shift-amount literal in `node->children[1]`, changed operator to `">>"`, guarded by `left->is_unsigned || (left_is_lit && lval >= 0L)`.

### Step 2: Memory Management

- Carefully sequenced pointer reassignments to avoid double-free: for the `2^N * x` case, moved the right operand to the left slot before freeing the old left operand.
- Each rule freed exactly the power-of-two literal that was no longer needed.
- New shift-amount literals were created via `ast_new(AST_INT_LITERAL, ...)` with properly initialized `decl_type` and `is_unsigned` fields.

### Step 3: Bootstrap Compatibility

- All declarations placed at the top of the outer block, including local variables for operator strings, operand pointers, power-of-two flags, shift amounts, and a temporary long.
- Used only `snprintf`, `strtol`, `strcmp` — all available in C0 (added in stage 83 and earlier).
- No VLAs or `//` comments; only C0-compatible block-scoped declarations.

### Step 4: Test Suite

- Added five new integration tests under `test/integration/`, each with a `.cflags` file specifying `-O1`:
  - `test_strength_reduce_mul_pow2`: Tests `x*2`, `x*4`, `x*8`, `x*16` for `x=3`, expecting `6 12 24 48`.
  - `test_strength_reduce_mul_pow2_commutative`: Tests `2*x`, `8*x` for `x=5`, expecting `10 40`.
  - `test_strength_reduce_div_pow2_unsigned`: Tests `x/2`, `x/4`, `x/8` for `unsigned x=48`, expecting `24 12 6`.
  - `test_strength_reduce_no_signed_div`: Tests that `x/2` for signed `x=-7` is NOT reduced (expecting `-3`, not `-4` from shift). Verifies correctness of the scope guard.
  - `test_strength_reduce_combined`: Tests composition with stage-143 constant folding: `x * (1 << 3)` folds to `x * 8`, then reduces to `x << 3`; and `x * (2 + 2)` folds to `x * 4`, then reduces to `x << 2`. Expecting `56 28` for `x=7`.

### Step 5: Version Bump

- Updated `src/version.c` to bump `VERSION_STAGE` to `"01460000"`.

## Final Results

All 2003 portable tests pass:
- 165 unit tests
- 1286 valid tests
- 261 invalid tests
- 120 integration tests (5 new + 115 from stage 145)
- 50 print-AST tests
- 100 print-tokens tests
- 21 print-asm tests

No regressions. All existing tests continue to pass.

Self-host verification: C0→C1→C2 cycle completed successfully. The compiler (C0, built with GCC) compiles itself to C1, which compiles itself to C2. All 2003 tests pass at every stage. No source changes were needed during bootstrap.

## Session Flow

1. Read the stage-146 specification, templates, and supporting documentation.
2. Reviewed the stage-142/143/145 optimizer infrastructure to understand AST-level optimization structure.
3. Examined the algebraic identity block from stage-145 to identify insertion point and patterns.
4. Implemented the strength-reduction block in `src/optimize.c` with three transformation rules.
5. Ensured C0 bootstrap compatibility: all declarations at block top, no VLAs, C0-compatible library calls.
6. Created five new integration tests covering multiply (right and commutative), divide (unsigned only), signed division guard, and composition scenarios.
7. Built the compiler and ran all tests; verified 2003 tests pass with no regressions.
8. Ran self-host cycle (C0→C1→C2) and confirmed all tests pass at every stage, no source changes needed.
9. Generated milestone summary, transcript, and README updates.
