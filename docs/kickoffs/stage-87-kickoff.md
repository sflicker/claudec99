# Stage 87 Kickoff: Controlled stdio.h File Position/Read Stubs

## Summary

Stage 87 adds stub declarations and macros to the controlled `test/include/stdio.h` header to support file positioning and reading operations. This stage adds three file-position control macros (`SEEK_SET`, `SEEK_CUR`, `SEEK_END`) and three function declarations (`fseek`, `ftell`, `fread`) that the ClaudeC99 compiler can recognize in user programs, allowing programs to compile and link against them without full implementations in the stdlib.

Core additions:
- Three `#define` macros for seek constants: `SEEK_SET 0`, `SEEK_CUR 1`, `SEEK_END 2`
- Three function declarations: `int fseek(FILE *, long, int)`, `long ftell(FILE *)`, `size_t fread(void *, size_t, size_t, FILE *)`
- No changes to tokenizer, parser, AST, or code generation

## Tokenizer Changes

No changes required. Existing tokenization handles `#define`, `int`, `long`, `void`, `size_t`, identifiers, numbers, punctuation, and parentheses correctly.

## Parser Changes

No changes required. Declarations and macro definitions are already parsed correctly.

## AST Changes

No changes required. No new language constructs introduced.

## Code Generation Changes

No changes required. The stage only adds header declarations; the runtime implementation is external.

## Test Plan

**Integration tests (2)**:
1. `test_fseek_ftell`: Tests file positioning with `fseek()` and `ftell()` on a file containing multiple lines. Opens a file, seeks to end, retrieves position, and verifies non-zero length.
2. `test_fread`: Tests bulk file reading with `fread()`. Opens a file, reads 5 bytes into a buffer, closes the file, and verifies the content matches expected data.

**Companion files**:
- Each test includes an `input.txt` file with test data
- Tests verify correct return values and data integrity

**Implementation steps**:
1. Add SEEK macros and function declarations to `test/include/stdio.h`
2. Create `test/integration/test_fseek_ftell/` test directory with `main.c` and `input.txt`
3. Create `test/integration/test_fread/` test directory with `main.c` and `input.txt`
4. Update `src/version.c` to increment VERSION_STAGE from `00860000` to `00870000`
5. Verify both integration tests compile and run successfully

