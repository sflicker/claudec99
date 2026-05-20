# stage-56-02 Transcript: Command-line Macro Definitions

## Summary

Stage 56-02 adds support for defining preprocessor macros from the compiler command line using the `-D` flag syntax. The feature allows two forms: `-DNAME` (expands to `1`) and `-DNAME=VALUE` (expands to the given value). These macros are injected into the macro table before source preprocessing begins, making them available to `#ifdef`, `#if`, and other preprocessor conditionals in the main source file. The implementation required extending the preprocessor API, updating the compiler driver to parse command-line arguments, and adding test harness support for a new `.cflags` companion file type to avoid collision with the existing `.args` file (which is reserved for runtime arguments).

## Changes Made

### Step 1: Preprocessor API

- Added `preprocess_with_defines()` function signature to `include/preprocessor.h`.
- Function accepts source text, source path, and a `defines` array of `"NAME"` or `"NAME=VALUE"` strings.
- Each define string is parsed and injected as a macro before source preprocessing.

### Step 2: Preprocessor Implementation

- Implemented `preprocess_with_defines()` in `src/preprocessor.c`.
- Initializes the macro table, iterates over the `defines` array, calls `macro_define()` for each entry with appropriate parsing (split on `=` separator to extract name and value).
- Delegates to existing `preprocess_internal()` after macro injection.
- Updated original `preprocess()` to call `preprocess_with_defines()` with `NULL` defines and count `0` for backward compatibility.

### Step 3: Compiler Driver

- Updated `main()` in `src/compiler.c` to parse `-D` command-line arguments.
- Collects `-Dname` or `-Dname=val` flags into a dynamic `defines` array during argument processing.
- Calls `preprocess_with_defines()` instead of `preprocess()`, passing the collected defines.
- Updated usage message to advertise `-DNAME[=VAL]` syntax.

### Step 4: Integration Test Harness

- Modified `test/integration/run_test.sh` to support `.cflags` companion files.
- Reads `BASENAME.cflags` into `COMPILER_FLAGS` array and passes flags to the compiler invocation.
- Updated documentation comment to describe `.cflags` purpose.

### Step 5: Integration Tests (Batch Runner)

- Modified `test/integration/run_tests.sh` to read and apply `.cflags` files.
- Builds `compiler_flags` array from the companion file and passes to both normal and error-test compiler invocations.
- Added inline documentation of the `.cflags` file format.

### Step 6: Test Cases

- Added `test/integration/test_cmdline_define_level/`: source conditionally compiles based on `#if LEVEL == 3`; `.cflags` contains `-DLEVEL=3`; `.status` expects exit code 42.
- Added `test/integration/test_cmdline_define_debug/`: source conditionally compiles based on `#ifdef DEBUG`; `.cflags` contains `-DDEBUG`; `.status` expects exit code 42.

## Final Results

All 1008 tests pass (1006 prior + 2 new integration tests).
- valid: 625
- invalid: 195
- integration: 29 (was 27, +2 new)
- print_ast: 39
- print_tokens: 99
- print_asm: 21

No regressions. Full test suite clean.

## Session Flow

1. Read specification and summarized completed changes.
2. Reviewed milestone and transcript templates.
3. Examined preprocessor and compiler driver code to understand existing patterns.
4. Reviewed integration test harness structure and recognized `.args` file conflict.
5. Documented changes across all modified subsystems.
6. Generated milestone summary focusing on subsystems touched and test results.
7. Generated transcript with detailed step-by-step implementation overview.
8. Updated README.md with new capabilities and corrected test counts.
9. Verified no grammar changes required (driver-only and preprocessor-level feature).

## Notes

The specification used `.args` as notation for command-line flags, but the integration test suite already uses `.args` files to specify runtime arguments (e.g., `test_argv_puts/.args` contains `hello world`). To avoid ambiguity and conflict, a new `.cflags` (compiler flags) companion file type was introduced. Both test harness scripts (`run_test.sh` and `run_tests.sh`) were updated to recognize and apply this new format.
