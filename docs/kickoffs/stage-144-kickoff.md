# Stage 144 Kickoff — Constant Unary Folding

## Summary of the spec

Complete constant-unary-folding by folding `AST_UNARY_OP` nodes whose single child is an `AST_INT_LITERAL` for the four unary operators: `-`, `+`, `!`, and `~`.

The `~` operator was already folded in stage 143 as a standalone block. This stage replaces that block with a unified rule that handles all four operators:

- `-val`: arithmetic negation; inherits operand type
- `+val`: unary plus (no-op); inherits operand type
- `!val`: logical NOT produces 0 or 1; result is always `TYPE_INT`
- `~val`: bitwise complement; inherits operand type

All folding is gated behind `-O1`; the default `-O0` path is unaffected.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation (optimizer) changes

In `src/optimize.c`, locate and replace the "Constant unary bitwise-NOT folding" block (the `~`-only block from stage 143) with a unified unary folding block.

The unified block follows this algorithm:

1. Match `AST_UNARY_OP` nodes with a single `AST_INT_LITERAL` child.
2. Extract the operator string and child value.
3. Compute the folded result:
   - `-`: negate the value
   - `+`: return the value unchanged
   - `!`: return 1 if value is 0, else 0
   - `~`: bitwise complement
4. Create a new `AST_INT_LITERAL` with the result.
5. Set `decl_type = TYPE_INT` and `is_unsigned = 0` for `!` (logical result).
6. Inherit `decl_type` and `is_unsigned` from the operand for `-`, `+`, and `~`.

The insertion point remains the same: after the logical short-circuit block and before the final `return node;` in `optimize_expr()`.

## Test plan

Four new integration tests, all using `-O1` in their `.cflags` file:

1. **test_fold_unary_minus**: unary negation of integer constants.
2. **test_fold_unary_plus**: unary plus (no-op) on integer constants.
3. **test_fold_unary_not**: logical NOT on integer constants.
4. **test_fold_unary_combined**: composition of unary and binary folds in a single pass.

All existing tests must continue to pass at both `-O0` and `-O1` via `./run_tests.sh`.

## Implementation steps

1. Edit `src/optimize.c`: replace the `~`-only block with the unified unary folding block.
2. Build and smoke test with a simple unary fold.
3. Add the four integration tests.
4. Run `./run_tests.sh` — verify all tests pass.
5. Run `./build.sh --mode=self-host` — verify self-host (C0→C1→C2) passes.
6. Bump `VERSION_STAGE` to `"01440000"` in `src/version.c`.
7. Update docs: mark "Constant unary folding" as complete in `docs/outlines/checklist.md`, run `update-supplemental-docs` skill, update `docs/self-compilation-report.md`.
