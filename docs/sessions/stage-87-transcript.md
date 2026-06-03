# stage-87 Transcript: File Position and Read Stubs

## Summary

Stage 87 extends the stub `stdio.h` header with file-positioning and read function declarations and macro constants. The stage adds three seek-position macros (`SEEK_SET`, `SEEK_CUR`, `SEEK_END`), two position-manipulation functions (`fseek` and `ftell`), and one read function (`fread`), all as declarations only. These stubs permit source code using standard file-positioning and byte-read operations to parse and compile, though the actual runtime behavior is provided by libc linking. Two integration tests verify that the declarations compile and that the underlying file I/O semantics work through the standard C library.

## Changes Made

### Step 1: stdio.h Header Stubs

- Added three file-positioning macros: `#define SEEK_SET 0`, `#define SEEK_CUR 1`, `#define SEEK_END 2`.
- Added function declarations:
  - `int fseek(FILE *, long, int);` — repositions the file pointer.
  - `long ftell(FILE *);` — returns the current file position.
  - `size_t fread(void *, size_t, size_t, FILE *);` — reads bytes from a file.
- All declarations placed in `test/include/stdio.h` after the existing `#define EOF (-1)` line.

### Step 2: Integration Tests

- Created `test/integration/test_fseek_ftell/` with:
  - `test_fseek_ftell.c`: Opens a test file, seeks to end with `fseek(f, 0L, SEEK_END)`, queries position with `ftell(f)`, verifies position is nonzero, returns 1 (success).
  - `input.txt`: Multi-line test file for position testing.
  - `test_fseek_ftell.status`: Exit code expectation (1).
- Created `test/integration/test_fread/` with:
  - `test_fread.c`: Opens a test file, reads 5 bytes into a buffer using `fread(buf, 1, 5, f)`, compares against "hello", returns 1 on match.
  - `input.txt`: File containing "hello" followed by newline.
  - `test_fread.status`: Exit code expectation (1).

### Step 3: Version Bump

- Updated `src/version.c` `VERSION_STAGE` from "00860000" (stage 86) to "00870000" (stage 87).

## Final Results

All 1284 tests pass (802 valid, 236 invalid, 82 integration, 43 print-AST, 100 print-tokens, 21 print-asm). Two new integration tests added; no regressions.

## Session Flow

1. Read stage specification and supporting test templates.
2. Reviewed existing stdio.h stub structure and integration test format.
3. Identified the stub declarations and macros to add (SEEK_SET, SEEK_CUR, SEEK_END, fseek, ftell, fread).
4. Updated test/include/stdio.h with macro and function-declaration additions.
5. Created two integration test subdirectories with C source and input files.
6. Updated src/version.c to reflect stage 87.
7. Built the compiler and ran the full test suite.
8. Confirmed 1284/1284 tests pass, including the 2 new integration tests.
