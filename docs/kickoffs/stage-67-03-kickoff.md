# Stage 67-03 Kickoff

## Summary

Stage 67-03 adds the `fgets` function declaration to the stub stdio.h header and introduces an integration test that exercises line-based file input. No compiler changes are required.

The stage adds one standard file I/O function declaration:
- `char *fgets(char *, int, FILE *);`

An integration test will open a named file ("input.txt"), read one line with fgets into a buffer, close the file, and verify the content matches the expected string with newline.

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

None required.

## Test Plan

1. Add fgets declaration to `test/include/stdio.h`
2. Create integration test directory `test/integration/test_fgets_line_input/`
3. Create test source file `test_fgets_line_input.c` that:
   - Includes stdio.h and string.h
   - Declares a 16-byte buffer for input
   - Opens "input.txt" for reading
   - Returns error code 1 if fopen fails
   - Reads one line with fgets(buf, 16, f)
   - Returns error code 2 if fgets fails (NULL result)
   - Closes the file with fclose
   - Returns 1 if buffer contents match "hello\n", 0 otherwise
4. Create data file `input.txt` with content "hello\n"
5. Create `test_fgets_line_input.status` with expected exit code 1
6. Run full test suite to verify the test passes

## Spec Issues Noted

- Line 32: `fget(buf, 16, f)` is a typo, should be `fgets(buf, 16, f)` (implementation will use correct spelling)

## Implementation Order

1. Update `test/include/stdio.h` with fgets declaration
2. Create integration test directory and test source file
3. Create `input.txt` data file with "hello\n"
4. Create `.status` file with expected exit code 1
5. Run full test suite
