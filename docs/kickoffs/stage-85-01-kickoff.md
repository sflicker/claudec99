# Stage 85-01 Kickoff

## Summary

Stage 85-01 adds five new function declarations to `test/include/string.h` and creates corresponding integration tests for each declaration. This stage requires no changes to the tokenizer, parser, AST, or code generation—it is a pure header and test addition stage.

### Functions Added

- `strncpy` — copy string with size limit
- `strncat` — concatenate string with size limit
- `strncmp` — compare strings with size limit
- `strcpy` — copy string
- `strrchr` — locate last occurrence of character

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

None required.

## Test Plan

Create six changes:

1. Add the five function declarations to `test/include/string.h`
2. Create integration test `test/integration/test_string_h_strncpy/` demonstrating `strncpy` usage and validating its presence
3. Create integration test `test/integration/test_string_h_strncat/` demonstrating `strncat` usage and validating its presence
4. Create integration test `test/integration/test_string_h_strncmp/` demonstrating `strncmp` usage and validating its presence
5. Create integration test `test/integration/test_string_h_strcpy/` demonstrating `strcpy` usage and validating its presence
6. Create integration test `test/integration/test_string_h_strrchr/` demonstrating `strrchr` usage and validating its presence

## Implementation Plan

1. Update `test/include/string.h` with the five new declarations
2. Create each integration test subdirectory with test code
3. Update `src/version.c` to set `VERSION_STAGE` to `"00850100"`
4. Run integration test suite to verify all tests pass

## Notes

The spec has a minor formatting issue where the "## Tests" heading appears inside a code fence block, but the intent is unambiguous: create integration tests demonstrating each declaration's usage.

## Planned File Changes

```
test/include/string.h                                    — add declarations
test/integration/test_string_h_strncpy/                  — new test
test/integration/test_string_h_strncat/                  — new test
test/integration/test_string_h_strncmp/                  — new test
test/integration/test_string_h_strcpy/                   — new test
test/integration/test_string_h_strrchr/                  — new test
src/version.c                                            — update VERSION_STAGE
```
