# stage-143 Transcript: Constant Integer Binary Folding

## Summary

Stage 143 implements constant integer binary folding in the AST-level optimizer: when both children of an `AST_BINARY_OP` are `AST_INT_LITERAL`, the optimizer computes the result at compile time and replaces the entire node with a single `AST_INT_LITERAL`. Arithmetic operators (`+`, `-`, `*`, `/`, `%`) with a division-by-zero guard, bitwise operators (`&`, `|`, `^`, `<<`, `>>`), relational operators (`<`, `<=`, `>`, `>=`, `==`, `!=`), logical short-circuit operators (`&&`, `||`), and unary bitwise-NOT (`~`) are all handled. Result types follow C semantics: relational and logical produce `TYPE_INT` with value 0 or 1; arithmetic and bitwise results inherit their type from the left operand. All folding is gated behind `-O1`; existing `-O0` behavior is unchanged. Six new integration tests verify folding correctness, nested expression folding, and division-by-zero guard behavior.

## Changes Made

### Step 1: Includes in `src/optimize.c`

Added three new includes at the top of the file:
- `#include <stdio.h>` — for `snprintf()`
- `#include <string.h>` — for `strcmp()`
- `#include "util.h"` — for `util_strdup()`

### Step 2: Binary Folding Rule

Inserted a constant-folding block into `optimize_expr()` after the generic child-recursion loop and before the final `return node;`. The rule:
- Checks if the node is `AST_BINARY_OP` with two children both `AST_INT_LITERAL`
- Parses both operands via `strtol()` (base 0 for auto-detect)
- Evaluates the operator (`+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`, `<`, `<=`, `>`, `>=`, `==`, `!=`)
- Handles division and modulo by zero by skipping the fold (leaves the node in place for codegen to handle)
- Tracks boolean results for relational/logical operators
- Creates a new `AST_INT_LITERAL` node with formatted result string via `util_strdup()`
- Sets result type to `TYPE_INT` for boolean results, or inherits from left operand for arithmetic/bitwise
- Frees the original node and returns the folded literal

### Step 3: Logical Short-Circuit Folding

Inserted a second rule block for `&&` and `||` operators. The rule:
- Folds `0 && X` to `0` (short-circuit, X not evaluated)
- Folds `nonzero || X` to `1` (short-circuit, X not evaluated)
- When the left operand is non-triggering for short-circuit, folds only if the right is also a literal
- Returns `TYPE_INT` with value 0 or 1

### Step 4: Unary Bitwise-NOT Folding

Inserted a third rule block for `AST_UNARY_OP` nodes with operator `~` and a single `AST_INT_LITERAL` child:
- Parses the operand via `strtol()`
- Computes bitwise complement: `~val`
- Creates new literal with formatted result via `util_strdup()`
- Inherits type and unsigned-ness from the child operand
- Frees the original node and returns the folded literal

### Step 5: Integration Tests

Created 6 new integration test directories under `test/integration/`:

- **test_fold_arithmetic**: verifies `+`, `-`, `*`, `/`, `%` folding to correct values
  - `.cflags`: `-O1`
  - Expected output: `7 7 42 3 1`

- **test_fold_bitwise**: verifies `&`, `|`, `^`, `<<`, `>>`, `~` folding
  - `.cflags`: `-O1`
  - Expected output: `240 255 240 32 32 -1`

- **test_fold_relational**: verifies `<`, `<=`, `>`, `>=`, `==`, `!=` produce 0 or 1
  - `.cflags`: `-O1`
  - Expected output: `1 1 1 0 1 1`

- **test_fold_logical**: verifies `&&` and `||` truth tables
  - `.cflags`: `-O1`
  - Expected output: `1 0 0 0 1 1 0`

- **test_fold_divzero_skipped**: verifies compiler doesn't crash on `-O1` (division by zero is left unfolded)
  - `.cflags`: `-O1`
  - Expected output: *(empty, exit code 0)*

- **test_fold_nested**: verifies nested expressions fold fully: `(1+2)*(3+4)=21`, `1<<(2+1)=8`
  - `.cflags`: `-O1`
  - Expected output: `21 8`

### Step 6: Version

Modified `src/version.c`: bumped `VERSION_STAGE` from `"01420000"` to `"01430000"`.

## Implementation Flow

1. Read stage 143 spec and confirmed no grammar changes, no new tokens, no parser changes, no codegen changes.
2. Added three `#include` directives to `src/optimize.c`.
3. Implemented binary folding rule with 15 operator cases and div-by-zero guard.
4. Implemented logical short-circuit folding with `0 && X` and `nonzero || X` rules.
5. Implemented unary `~` folding rule.
6. Built with cmake; compilation successful on first attempt.
7. Created 6 integration test directories with `.c`, `.cflags`, and `.expected` files.
8. Ran full test suite: all 1988 tests pass (165 unit, 1286 valid, 261 invalid, 105 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
9. Ran self-host bootstrap (C0→C1→C2):
   - All stages pass cleanly
   - All tests pass at every stage
   - No source changes needed during bootstrap
10. Updated `src/version.c` version string.

## Final Results

**Portable test suite**: All 1988 tests pass.
- Unit: 165
- Valid: 1286
- Invalid: 261
- Integration: 105 (including 6 new fold tests)
- Print-AST: 50
- Print-tokens: 100
- Print-asm: 21

**Self-host bootstrap**:
- C0 (GCC-built): `00.03.01430000.01077` — all tests pass
- C1 (C0-built): `00.03.01430000.01078` — all tests pass
- C2 (C1-built): `00.03.01430000.01079` — all tests pass
- All 1988 portable tests pass at every stage

**Key verification**: The `-O0` and `-O1` paths both produce functionally correct code; the difference is purely in whether constant folding is applied. Nested expressions fold correctly in a single bottom-up pass. Division by zero is skipped and left for codegen to handle (matching unoptimized semantics).

## Notes

**Design decisions**:
- Result type inheritance: arithmetic/bitwise results inherit type from left operand (conservative approximation of usual arithmetic conversions; a future stage may refine to full type-ranking logic).
- Division-by-zero guard: instead of emitting a compiler error, the node is left unfolded so codegen emits the divide instruction (which will trap or produce undefined behavior at runtime — matching unoptimized semantics).
- Logical short-circuit: when the left operand is constant, the rule applies only if it triggers short-circuit (0 for `&&`, nonzero for `||`) or if the right operand is also constant.
- Unary `~`: inherits type and unsigned-ness from child (no special casing).
- String literals for boolean results: "0" and "1" are string constants and do not require heap allocation.

**Verification of folding**:
- Smoke test on `test_fold_arithmetic`: compiled with `-O1`, ran successfully, output matched expected.
- Nested folding test: `(1+2)*(3+4)` correctly folded to `21` (bottom-up: `1+2→3`, `3+4→7`, `3*7→21`).
- All 6 integration tests pass with expected output.

**No bootstrapping issues encountered**.
