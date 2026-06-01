# stage-84-02 Transcript: stdlib.h exit()

## Summary

stage-84-02 adds the `exit()` function declaration to the stub `stdlib.h` header. This is a minimal addition that enables C99 programs to call the standard library exit function to terminate with a specified exit code. No language constructs, grammar changes, or code generation modifications are required—the existing call infrastructure already supports calling external libc functions via function declarations.

## Changes Made

### Step 1: Update stdlib.h Header

- Added `void exit(int status);` function declaration to `test/include/stdlib.h`.
- Placed after existing stdlib declarations (`malloc`, `realloc`, `calloc`, `free`).

### Step 2: Add Test Case

- Created new valid test `test/valid/test_stdlib_exit__42.c` that includes `<stdlib.h>` and calls `exit(42)`.
- Test validates that the exit code is correctly propagated (expects exit code 42).

### Step 3: Update Version

- Updated `VERSION_STAGE` in `src/version.c` from `"00840000"` to `"00840200"`.

## Final Results

All tests pass. Full aggregate results: 1261 tests pass (789 valid, 235 invalid, 73 integration, 43 print-AST, 100 print-tokens, 21 print-asm). No regressions. The new test was added to the valid suite; prior count was 788 valid tests.

## Session Flow

1. Read stage spec defining the exit() addition.
2. Reviewed existing stdlib.h stub in test/include/.
3. Identified required changes: header declaration and test case.
4. Added void exit(int status); declaration to stdlib.h.
5. Created new test case validating exit(42) behavior.
6. Updated version string to stage 84-02.
7. Built and ran full test suite.
8. Verified all tests pass with no regressions.
