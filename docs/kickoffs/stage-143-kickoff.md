# Stage 143 — Constant Integer Binary Folding

**Kickoff:** 2026-06-19

## Summary of the Spec

Stage 143 implements the first real optimization rule in the stage-142 optimizer infrastructure: **constant integer binary folding**. When both children of an `AST_BINARY_OP` node are `AST_INT_LITERAL`, the compiler computes the result at compile time and replaces the entire binary operation with a single `AST_INT_LITERAL`. The stage also handles unary bitwise-NOT (`~`) folding on a constant operand.

**Scope:** Changes to `src/optimize.c` only; all folding is gated behind `-O1`.

**Key properties:**
- Bottom-up optimization: children are recursively optimized before the parent sees them
- Integer literals stored as decimal strings in `node->value`, parsed with `strtol(..., NULL, 0)`
- Result type convention: relational/logical results are `TYPE_INT` with value 0 or 1; arithmetic/bitwise results inherit left operand type
- Division and modulo by zero are not folded (node left in place for runtime behavior)
- Logical AND/OR implement C short-circuit semantics

## Required Tokenizer Changes

**None.** All operators (`+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`, `<`, `<=`, `>`, `>=`, `==`, `!=`, `&&`, `||`, `~`) already tokenized.

## Required Parser Changes

**None.** All binary and unary operators already parsed into `AST_BINARY_OP` and `AST_UNARY_OP` nodes.

## Required AST Changes

**None.** Existing node types used as-is:
- `AST_BINARY_OP` with `child_count == 2`
- `AST_UNARY_OP` with `child_count == 1` (for `~`)
- `AST_INT_LITERAL` for result nodes
- `node->value` for operator string
- `node->decl_type` and `node->is_unsigned` for type tracking

## Required Code Generation Changes

**None.** Folding happens during optimization (after parsing, before codegen). Codegen sees only the optimized AST.

## Code Changes to `src/optimize.c`

### Includes

Add three includes (if not already present):
```c
#include <stdio.h>    /* snprintf */
#include <string.h>   /* strcmp   */
#include "util.h"     /* util_strdup */
```

### Binary Folding Rule

Insert after the child-recursion loop and before the final `return node;` in `optimize_expr`:

1. Check: `node->type == AST_BINARY_OP` and both children `AST_INT_LITERAL`
2. Parse left and right operands with `strtol`
3. Dispatch on operator string: arithmetic (+, -, *, /, %), bitwise (&, |, ^, <<, >>), relational (<, <=, >, >=, ==, !=)
4. Guard division/modulo by zero: skip fold if right operand is 0
5. Compute result
6. For relational/logical: set `result_is_bool = 1` (result type becomes `TYPE_INT`, 0 or 1)
7. Format result with `snprintf`, allocate with `util_strdup`
8. Create `AST_INT_LITERAL` node, inherit type from left operand (or `TYPE_INT` for booleans)
9. Free original node and return new literal

### Logical Short-Circuit Folding Rule

Insert adjacent to binary folding:

1. Check: `node->type == AST_BINARY_OP` for `&&` or `||`
2. For `&&`: if left is constant 0, fold to 0 immediately; if left is nonzero and right is also constant, fold based on right value
3. For `||`: if left is nonzero, fold to 1 immediately; if left is 0 and right is also constant, fold based on right value
4. Short-circuit: never fold if right operand is non-constant (preserves evaluation order)

### Unary Bitwise-NOT Folding Rule

Insert adjacent to binary folding:

1. Check: `node->type == AST_UNARY_OP`, `node->value == "~"`, and child `AST_INT_LITERAL`
2. Parse child value with `strtol`, compute bitwise NOT
3. Format with `snprintf`, allocate with `util_strdup`
4. Create `AST_INT_LITERAL` node, inherit type and sign from child
5. Free original node and return new literal

## Test Plan

### Integration Tests (6 new tests under `test/integration/`)

All tests use `.cflags: -O1` to enable the optimizer.

1. **test_fold_arithmetic**: addition, subtraction, multiplication, division, modulo
   - Expected: `7 7 42 3 1`

2. **test_fold_bitwise**: bitwise AND, OR, XOR, left shift, right shift, unary NOT
   - Expected: `240 255 240 32 32 -1`

3. **test_fold_relational**: all six relational operators
   - Expected: `1 1 1 0 1 1`

4. **test_fold_logical**: AND and OR truth tables
   - Expected: `1 0 0 0 1 1 0`

5. **test_fold_divzero_skipped**: compiler does not crash on valid code with `-O1`
   - Expected: exit 0 (no output)

6. **test_fold_nested**: nested expressions fold in one pass: `(1+2)*(3+4)` → `21`, `1<<(2+1)` → `8`
   - Expected: `21 8`

### Regression Testing

- Run `./run_tests.sh` to verify all existing tests still pass at both `-O0` and `-O1`
- Self-host: `./build.sh --mode=self-host` (C0→C1→C2)

## Bootstrap Requirements

Source must compile cleanly under the C0 compiler (previous stage binary):
- No VLAs
- No `//` comments (block comments only)
- Declarations at block top
- Nested blocks `{ ASTNode *lit = ...; ... }` permitted
- `util_strdup` available (stage 83+)
- `snprintf` from `<stdio.h>` (already permitted)

## Implementation Order

1. Edit `src/optimize.c`: add includes
2. Edit `src/optimize.c`: add binary folding rule
3. Edit `src/optimize.c`: add logical short-circuit folding rule
4. Edit `src/optimize.c`: add unary `~` folding rule
5. Build: `cmake --build build`
6. Smoke test one file manually
7. Create 6 integration test directories
8. Run `./run_tests.sh` — verify all pass
9. Run `./build.sh --mode=self-host` — C0→C1→C2
10. Bump `VERSION_STAGE` to `"01430000"` in `src/version.c`
11. Update checklist and docs

## Notes & Decisions

- **Division by zero:** Not folded; binary node left in place, codegen emits the instruction, runtime trap (same as unoptimized)
- **Short-circuit semantics:** Logical operators only fold when semantically safe: `0 && X` always folds to 0; `nonzero || X` always folds to 1; otherwise only fold if both operands are constant
- **Type inheritance:** Arithmetic/bitwise results inherit left operand type; relational/logical results are always `TYPE_INT` 0 or 1
- **Memory:** `util_strdup` allocates heap copy; string literals `"0"` and `"1"` are constants and not freed
- **No grammar changes:** Syntax unchanged; this is a pure optimization pass

## Files to Modify

- `src/optimize.c` (main changes)
- `test/integration/test_fold_arithmetic/` (new)
- `test/integration/test_fold_bitwise/` (new)
- `test/integration/test_fold_relational/` (new)
- `test/integration/test_fold_logical/` (new)
- `test/integration/test_fold_divzero_skipped/` (new)
- `test/integration/test_fold_nested/` (new)
- `src/version.c` (version bump only)

---

**Ready to begin implementation.**
