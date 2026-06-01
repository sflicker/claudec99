# Stage 84 Kickoff

## Summary of the spec

Stage 84 adds the three standard stream extern declarations to the test's controlled `stdio.h` header. The task is straightforward: declare `stdin`, `stdout`, and `stderr` as external FILE pointers in `test/include/stdio.h`.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None.

## Test plan

A new integration test will verify that the standard stream declarations are present and usable. The test will:

1. Include `<stdio.h>`
2. Call `fprintf()` with `stdout` and `stderr`
3. Check that all three pointers (`stdin`, `stdout`, `stderr`) are non-null
4. Return 1 (success) if all checks pass

## Implementation plan

1. Update `test/include/stdio.h` to add the three extern declarations
2. Update `src/version.c` to stage 00840000
3. Create new integration test `test/valid/test_stdio_streams__1.c`
4. Compile and run the test to verify the declarations are accessible
