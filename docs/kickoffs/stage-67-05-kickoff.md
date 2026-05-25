# Stage 67-05 Kickoff

## Summary of the Spec

Stage 67-05 adds support for formatted string output via `snprintf()`. This stage:

- Adds the `snprintf` function declaration to `test/include/stdio.h`: `int snprintf(char *, size_t, const char *, ...);`
- Adds an integration test `test_snprintf_include` that:
  - Declares a 16-byte buffer
  - Calls `snprintf(buf, sizeof(buf), "%d", 42)` to format an integer
  - Uses `strcmp()` to verify the result equals `"42"`
  - Returns 1 (true) on successful comparison

## Required Tokenizer Changes

None. The `snprintf` identifier and syntax are already supported by the existing tokenizer.

## Required Parser Changes

None. The `snprintf` call uses the existing external call syntax with variadic arguments, which is already supported.

## Required AST Changes

None. `snprintf` is an external function call with variadic parameters (`...`), and the AST already represents external calls with variadic arguments (as demonstrated by existing tests with functions like `printf` and `fprintf`).

## Required Code Generation Changes

None. `snprintf` uses the existing variadic external-call code generation path. The codegen already handles:
- External function calls with variable argument counts
- Passing `char*` and `size_t` arguments
- Variadic formatting strings

## Test Plan

1. Add the `snprintf` declaration to `test/include/stdio.h`
2. Create the integration test directory structure:
   - `test/integration/test_snprintf_include/test_snprintf_include.c`
   - `test/integration/test_snprintf_include/test_snprintf_include.status`
3. Implement the test that:
   - Declares `char buf[16]`
   - Calls `snprintf(buf, sizeof(buf), "%d", 42)`
   - Calls `strcmp(buf, "42")` and checks if the result equals 0
   - Returns 1 (true) on successful comparison
4. Expected behavior: The test should return 1 (true) on successful string match
