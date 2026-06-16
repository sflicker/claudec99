# Stage 132 Kickoff — Pointer Equality With Non-Null Constants

## Overview

Stage 132 extends cc99 to accept pointer equality (`==` and `!=`) comparisons
against non-zero integer constant operands, matching the extension behavior of
GCC and Clang. This is separate from standard C99 null pointer constants (`0`);
cc99 will now compile expressions like `ptr == 1` and `ptr != 2` by comparing
address values.

## Spec Summary

The regression program (`test_pointer_eq_integer_constants__15.c`) currently
fails to compile with error `error: comparing pointer with non zero integer`.
GCC and Clang accept the same code with pointer/integer comparison warnings and
return `15`.

| Test | Expression | Current cc99 | Required cc99 |
|------|-----------|--------------|---------------|
| +1 | `one == 1` (pointer `(int *)1` vs int `1`) | error | accept |
| +2 | `one != 2` (pointer `(int *)1` vs int `2`) | error | accept |
| +4 | `two == 2` (pointer `(int *)2` vs int `2`) | error | accept |
| +8 | `two != 1` (pointer `(int *)2` vs int `1`) | error | accept |
| **Total** | exit code | fail to compile | `15` |

The second test (`test_pointer_eq_nonnull_constants__63.c`) verifies that
standard pointer/pointer comparisons continue to work and already passes on
current cc99, ensuring no regression.

## Required Changes

### 1. Tokenizer Changes

None.

### 2. Parser Changes

None.

### 3. AST Changes

None.

### 4. Code Generation Changes (`src/codegen.c`)

In the `is_pointer_cmp` block (semantic analysis for equality operators):

1. Add an `is_integer_constant()` helper function to check whether an AST node
   is an `AST_INT_LITERAL` (a constant integer in the parsed AST).

2. Relax the pointer/integer equality check: instead of rejecting all
   pointer/integer pairs, allow `==` and `!=` when the integer operand is a
   constant (detected via `is_integer_constant()`).

3. Accept both value `0` (existing null pointer constant path) and non-zero
   values through this extension.

4. The comparison generates `cmp` and `setb`/`seta` instructions at runtime,
   comparing the pointer value against the integer constant converted to
   pointer-width (64-bit on SysV AMD64).

### 5. Version Bump (`src/version.c`)

Change `VERSION_STAGE` from `"13100000"` to `"13200000"`.

## Test Plan

- Add `test/valid/pointers/test_pointer_eq_integer_constants__15.c` — the
  regression program from the spec (four if-condition checks, expected exit
  code 15).

- Add `test/valid/pointers/test_pointer_eq_nonnull_constants__63.c` — strict
  pointer operand control test verifying pointer/pointer and
  pointer-to-constant-cast comparisons continue to work (expected exit code 63).

- Run full test suite to verify no regressions in existing pointer tests.

- Self-host `./build.sh --mode=self-host`.

## Implementation Order

1. Code generation fix in `src/codegen.c` (helper + condition update)
2. Version bump (`src/version.c`)
3. New regression tests (two files under `test/valid/pointers/`)
4. Full test suite
5. Self-host cycle
6. Commit

## Notes & Decisions

- The fix is purely semantic analysis and code generation; no tokenizer,
  parser, or AST structure changes.

- Non-constant integer expressions are not required to be accepted in this
  stage (e.g., `ptr == variable`). Only integer constant expressions are in
  scope.

- The extension does not affect relational comparisons (`<`, `>`, `<=`, `>=`)
  or implicit pointer initialization. Those remain outside scope.

- Constant folding (e.g., `(int *)1 == 1` evaluated at compile time) is
  desirable but not required for spec acceptance.

- GCC and Clang emit a `warning: comparison between pointer and integer`
  diagnostic. cc99 may choose to emit a similar diagnostic or silently accept
  the extension, per implementation choice.
