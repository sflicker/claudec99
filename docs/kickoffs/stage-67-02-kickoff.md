# Stage 67-02 Kickoff

## Summary

Stage 67-02 adds file I/O function declarations to the stub stdio.h header and introduces an integration test that exercises basic file operations. No compiler changes are required.

The stage adds three standard file I/O function declarations:
- `FILE *fopen(const char *, const char *);`
- `int fclose(FILE *);`
- `int fgetc(FILE *);`

An integration test will open a named file ("input.txt"), read one character, close the file, and return the character's ASCII value (65 for 'A').

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

None required.

## Test Plan

1. Add fopen, fclose, fgetc declarations to `test/include/stdio.h`
2. Create integration test directory `test/integration/test_fopen_fgetc_fclose/`
3. Create test source file `test_fopen_fgetc_fclose.c` that:
   - Includes stdio.h
   - Opens "input.txt" for reading
   - Returns error code 1 if fopen fails
   - Reads one character with fgetc
   - Closes the file with fclose
   - Returns the character's ASCII value (65 for 'A')
4. Create data file `input.txt` with content "A\n"
5. Create `test_fopen_fgetc_fclose.status` with expected exit code 65
6. Update `test/integration/run_tests.sh` to cd into test directory before running binary (for relative path resolution)
7. Run full test suite to verify the test passes

## Spec Issues Noted

- Line 32: `fgetd(f)` is a typo, should be `fgetc(f)`
- Line 14: "integeration test" is a typo, should be "integration test"
- Line 20: "main .f file" is a typo, should be "main .c file"
- Path resolution: `fopen("input.txt", "r")` requires test runner to execute from test directory

## Implementation Order

1. Update `test/include/stdio.h` with function declarations
2. Create integration test directory and test source file
3. Create `input.txt` data file with "A\n"
4. Create `.status` file with expected exit code 65
5. Update `test/integration/run_tests.sh` to cd into test directory before running binaries
6. Run full test suite
