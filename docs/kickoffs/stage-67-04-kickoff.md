# Stage 67-04 Kickoff

## Summary of the Spec

Stage 67-04 adds support for file output via `fprintf()`. This stage:

- Adds the `fprintf` function declaration to `test/include/stdio.h`: `int fprintf(FILE *, const char *, ...);`
- Adds an integration test `test_fprintf_file_output` that:
  - Opens a file for writing
  - Writes formatted output using `fprintf()`
  - Closes the file
  - Reopens it for reading
  - Reads the output back with `fgets()`
  - Compares the result with `strcmp()` to verify correctness

Note: The spec contains a typo in the test code (line 43: `strcmp("buf", "42\n")` should be `strcmp(buf, "42\n")`). The implementation will use the corrected form with the variable name instead of the string literal.

## Required Tokenizer Changes

None. The `fprintf` identifier and syntax are already supported by the existing tokenizer.

## Required Parser Changes

None. The `fprintf` call uses the existing external call syntax with variadic arguments, which is already supported.

## Required AST Changes

None. `fprintf` is an external function call with variadic parameters (`...`), and the AST already represents external calls with variadic arguments (as demonstrated by existing tests with functions like `printf`).

## Required Code Generation Changes

None. `fprintf` uses the existing variadic external-call code generation path. The codegen already handles:
- External function calls with variable argument counts
- Passing `FILE*` pointers as arguments
- Variadic formatting strings

## Test Plan

1. Add the `fprintf` declaration to `test/include/stdio.h`
2. Create the integration test directory structure:
   - `test/integration/test_fprintf_file_output/test_fprintf_file_output.c`
   - `test/integration/test_fprintf_file_output/test_fprintf_file_output.status`
3. Implement the test that:
   - Opens `out.txt` for writing
   - Calls `fprintf(f, "%d\n", 42)`
   - Closes the file
   - Reopens `out.txt` for reading
   - Reads the output with `fgets(buf, sizeof(buf), f)`
   - Compares the result with `strcmp(buf, "42\n")`
   - Returns appropriate exit codes for each failure point
4. Expected behavior: The test should return 1 (true) on successful comparison
