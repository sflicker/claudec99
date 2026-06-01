# stage 84-02 Kickoff

## Summary

Add `void exit(int status)` to the stdlib.h stub header. The `exit()` function is a standard C library function that terminates program execution with a specified exit code. The compiler already supports calling extern functions, so only the header declaration is needed.

## Spec Summary

- Add `void exit(int status);` to `test/include/stdlib.h`
- Create a test that includes `<stdlib.h>`, calls `exit(42)`, and verifies exit code 42
- Update version to "00840200"

## Required Tokenizer Changes

None. No new tokens needed.

## Required Parser Changes

None. No new syntax needed.

## Required AST Changes

None. The AST already supports function calls and extern functions.

## Required Code Generation Changes

None. The codegen already supports calling extern functions.

## Test Plan

1. Verify stdlib.h contains the `exit()` function declaration
2. Create test_stdlib_exit__42.c that:
   - Includes `<stdlib.h>`
   - Calls `exit(42)` from main()
   - Expects exit code 42
3. Run the test and verify it returns exit code 42

## Implementation Order

1. Add `void exit(int status);` to test/include/stdlib.h
2. Create test/valid/test_stdlib_exit__42.c
3. Update src/version.c with new VERSION_STAGE

## Ambiguities and Decisions

None identified. The spec is complete and straightforward.
