# stage-148 Transcript: Negation Folding

## Summary

Stage 148 extends the stage-142 optimizer with negation folding, reducing `-(-x)` to `x` for non-constant expressions `x`. This is the arithmetic analogue of the `!!x` (double logical NOT) rule added in Stage 147. The implementation adds a single new rule block to `src/optimize.c` that detects double unary-minus chains and returns the inner operand directly, with careful memory management to prevent double-free. All folding is gated behind `-O1`; the default `-O0` path remains unaffected.

The stage completes the "Negation folding" checklist item from the project outline. When `x` is a compile-time integer literal, stage-144's two-pass constant unary folding already reduces `-(-const)` to the original value, so this rule fires only for non-constant operands. The implementation reuses the memory-management pattern from stage-147, substituting `"-"` for `"!"` and returning `x` directly instead of building a `!=` node.

## Changes Made

### Step 1: Optimizer

- Located the closing brace of the stage-147 "Double logical NOT" block (after the `return neq;` sequence) in `src/optimize.c`.
- Inserted a new "Double arithmetic negation" rule block that detects patterns where both the outer and inner operands are `AST_UNARY_OP("-")` nodes with a non-literal child.
- The block declares `inner`, `x`, and `fire` at the top of the outer `if` for C0 compatibility.
- Nulls `inner->children[0]` before calling `ast_free(node)` to prevent double-free of the inner operand.
- Returns the inner operand `x` directly without allocating a new node.
- Positioned the block before the stage-145 "Algebraic identity folding" comment as specified.

### Step 2: Version

- Bumped `VERSION_STAGE` to `"01480000"` in `src/version.c`.

### Step 3: Tests

- Added three new integration tests under `test/integration/`:
  - **test_neg_fold_double_minus**: Tests `-(-x)` for positive and negative variables, and `-(-0)`, `-(-7)` for constants (which are handled by stage-144).
  - **test_neg_fold_triple_minus**: Tests `-(-(-x))` to verify that the bottom-up pass correctly reduces the inner pair first, leaving a single negation.
  - **test_neg_fold_combined**: Tests composition of negation folding with algebraic identity rules: `-(-x) + 0 → x`, `-(-x) * 1 → x`, `-(-x) - x → 0`.
- All tests use `-O1` flag in their `.cflags` file.
- All tests produce expected output that verifies correct optimization.

### Step 4: Documentation

- Updated `docs/outlines/checklist.md` to mark the "Negation folding" item complete and annotate it with Stage 148.
- Updated `docs/self-compilation-report.md` to record the stage-148 self-host run with full test counts.

## Final Results

All 2012 portable tests pass:
- 165 unit tests
- 1286 valid tests
- 261 invalid tests
- 129 integration tests (126 existing + 3 new)
- 50 print-AST tests
- 100 print-tokens tests
- 21 print-asm tests

Self-host C0→C1→C2 cycle verified: 2012/2012 tests pass at every stage. No source changes needed during bootstrap.

## Session Flow

1. Read stage spec and kickoff notes
2. Reviewed stage-147 transcript for memory-management patterns
3. Located insertion point in `src/optimize.c` between stage-147 and stage-145 blocks
4. Implemented double-negation folding rule block with memory-safe child detachment
5. Built with `cmake --build build`
6. Ran smoke test with simple `-O1` double-negation expression
7. Created three new integration tests with `-O1` flag
8. Ran full test suite with `./run_tests.sh` — all 2012 tests pass
9. Ran self-host bootstrap cycle with `./build.sh --mode=self-host` — C0→C1→C2 verified
10. Bumped version in `src/version.c` and updated documentation
11. Verified all artifacts generated for final commit

## Notes

- The new rule fires only for non-constant operands; constant cases are already handled by stage-144's two-pass constant unary folding mechanism.
- The memory-management pattern is identical to the stage-147 `!!x` block: detach the grandchild from the inner node before freeing to avoid double-free of the kept value.
- No grammar changes were needed; the optimizer works entirely within the existing AST structure.
- The folding rule correctly composes with other optimizer rules (algebraic identity, strength reduction) in a single bottom-up pass.
