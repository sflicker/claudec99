# stage-147 Transcript: Boolean and Logical Simplification

## Summary

Stage 147 extends the stage-142 AST-level optimizer with boolean and logical simplification rules, firing under `-O1` in the `optimize_expr` function. Two independent rule blocks are implemented: a double-NOT simplification (`!!x → x != 0` for non-literal `x`) and a set of binary logical/boolean constant-folding rules (`x && 0 → 0`, `x || nonzero → 1`, `x && nonzero → (x != 0)`, `x || 0 → (x != 0)`). The left-operand-literal cases (e.g., `0 && x`, `nonzero || x`) are already handled by stage-143's short-circuit folding and are not re-implemented. All transformations mutate or replace AST nodes in place with careful memory management to avoid double-free bugs.

## Changes Made

### Step 1: Double-NOT Simplification Block

- Inserted a new rule block in `optimize_expr` after the stage-144 unary folding block and before the stage-145 algebraic identity block.
- Detects the pattern: outer `AST_UNARY_OP` with operator `"!"` whose child is also `AST_UNARY_OP` with operator `"!"` with a non-literal child.
- The constant case (`!!const`) is skipped because stage-144 unary folding already applies two folds in sequence.
- Builds a new `AST_BINARY_OP("!=", x, AST_INT_LITERAL("0"))` via `ast_new` and `ast_add_child`.
- Nulls the inner node's child slot before `ast_free` to prevent double-free of the operand.
- Returns the new binary `!=` node.

### Step 2: Binary Boolean Simplification Block

- Inserted a new rule block in `optimize_expr` after the stage-146 strength reduction block and before the final `return node`.
- Guards with `AST_BINARY_OP` type check and exactly two children.
- Implements four transformation rules, all checking the right operand's literal value:

  1. **`x && 0 → 0`**: detects `&&` operator with right operand `= 0`, frees entire subtree, returns fresh `AST_INT_LITERAL("0")`.
  2. **`x || nonzero → 1`**: detects `||` operator with right operand nonzero, frees entire subtree, returns fresh `AST_INT_LITERAL("1")`.
  3. **`x && nonzero → (x != 0)`**: detects `&&` operator with right operand nonzero, frees right child, installs new `AST_INT_LITERAL("0")` in right slot, mutates operator to `"!="`.
  4. **`x || 0 → (x != 0)`**: detects `||` operator with right operand `= 0` (right child is already zero), mutates operator to `"!="`, leaves tree structure intact.

- All four rules use early `return` on match.

### Step 3: Memory Management

- For `x && 0` and `x || nonzero`: freed the entire subtree via `ast_free(node)` to avoid dangling pointers.
- For `x && nonzero`: freed the old right operand before installing a new literal.
- For `x || 0`: right child is reused (already the zero literal); no free needed.
- Ensured no double-frees by careful pointer tracking and conditional frees.

### Step 4: Bootstrap Compatibility

- All declarations placed at the top of the outer block.
- Used only C0-compatible standard library calls: `strcmp`, `strtol`.
- No VLAs or C99-style intermediate declarations.

### Step 5: Test Suite

- Added six new integration tests under `test/integration/`:
  - `test_bool_simplify_and_zero`: Tests `(x && 0)` simplifies to 0.
  - `test_bool_simplify_or_nonzero`: Tests `(x || 1)` simplifies to 1.
  - `test_bool_simplify_and_one`: Tests `(x && 1)` simplifies to `(x != 0)`.
  - `test_bool_simplify_or_zero`: Tests `(x || 0)` simplifies to `(x != 0)`.
  - `test_bool_double_not`: Tests `!!x` simplifies to `(x != 0)`.
  - `test_bool_simplify_combined`: Tests composition of multiple rules and interaction with stage-143 short-circuit folding.

- All tests run at both `-O0` (optimizer off, no simplification) and `-O1` (optimizer on, rules fire).

### Step 6: Version Bump

- Updated `src/version.c` to bump `VERSION_STAGE` to `"01470000"`.

## Final Results

All 126 portable tests pass:
- 165 unit tests
- 1286 valid tests
- 261 invalid tests
- 126 integration tests (6 new + 120 from stage 146)
- 50 print-AST tests
- 100 print-tokens tests
- 21 print-asm tests

No regressions. All existing tests continue to pass.

Self-host verification: C0→C1→C2 cycle completed successfully. The compiler (C0, built with GCC) compiles itself to C1, which compiles itself to C2. All 126 tests pass at every stage. No source changes were needed during bootstrap.

## Session Flow

1. Read the stage-147 specification and supporting documentation.
2. Reviewed the stage-142/143/144/145/146 optimizer infrastructure to understand the AST-level optimization pipeline.
3. Examined existing rule blocks (unary folding, algebraic identity, strength reduction) to identify insertion points and patterns.
4. Implemented the double-NOT simplification block with pattern matching and new node construction.
5. Implemented the binary boolean simplification block with four independent transformation rules.
6. Ensured C0 bootstrap compatibility: all declarations at block top, no VLAs, C0-compatible library calls.
7. Created six new integration tests covering all four binary rules, the double-NOT rule, and composition scenarios.
8. Built the compiler and ran all tests; verified 126 tests pass with no regressions.
9. Ran self-host cycle (C0→C1→C2) and confirmed all tests pass at every stage, no source changes needed.
10. Generated milestone summary, transcript, and README updates.
