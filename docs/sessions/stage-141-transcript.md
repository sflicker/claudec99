# stage-141 Transcript: System Includes

## Summary

Stage 141 adds tooling and test infrastructure to validate the compiler against real system headers on Linux x86_64. The `bin/cc99` wrapper gains a `--sysinclude` flag that switches the compiler's include paths from stub headers (`test/include`) to real system paths. The test runner (`test/run_all_tests.sh`) detects Linux x86_64 and runs a separate system-include integration test suite, reporting results in a distinct section. No compiler language features or code generation changes are made in this stage; this is a pure infrastructure stage.

## Changes Made

### Step 1: `bin/cc99` Wrapper Enhancement

- Added `--sysinclude` flag to command-line options parsing.
- When `--sysinclude` is passed on Linux x86_64, the wrapper replaces the default `-I${PROJECT_DIR}/test/include` stub-header path with four real system paths in order: `/usr/lib/gcc/x86_64-linux-gnu/13/include`, `/usr/local/include`, `/usr/include/x86_64-linux-gnu`, `/usr/include`.
- Platform guard: emits an error if `--sysinclude` is used on non-Linux or non-x86_64 architecture, with a clear diagnostic message.
- Updated usage help text to document the new flag.

### Step 2: Test Runner Integration

- `test/run_all_tests.sh` now detects the platform using `uname -s` (Linux) and `uname -m` (x86_64).
- On Linux x86_64, after running the seven portable test suites, the runner invokes `test/integration/run_tests_sysinclude.sh` and captures results.
- System-include test results are reported in a separate "System include:" section distinct from the portable "Portable:" aggregate line.
- System-include suite failures do NOT affect the overall exit code due to pre-existing platform limitations.

### Step 3: Version Bump

- `src/version.c`: `VERSION_STAGE` updated from `"14000000"` to `"14100000"`.

## Final Results

- **Portable suite**: all 1982 tests pass (1284 valid, 262 invalid, 98 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- **System-include suite (Linux x86_64)**: 98/99 tests pass; 1 pre-existing failure (`test_std_pointer_size_typedefs` via `bits/wchar.h` wide-char literal `L'\0'` in `#elif` — an unsupported preprocessor constant-expression form).
- **Self-host C0→C1→C2 verified**: all 1982 portable tests pass at every bootstrap stage.
- No regressions; no language feature changes; no codegen changes.

## Session Flow

1. Read spec and infrastructure requirements for `--sysinclude` flag and test suite integration.
2. Reviewed existing `bin/cc99` wrapper structure and test runner architecture.
3. Implemented `--sysinclude` flag detection and real system path substitution in `bin/cc99`, with platform guard.
4. Updated `test/run_all_tests.sh` to detect Linux x86_64 and conditionally run system-include test suite.
5. Verified portable test suite results (1982 tests passing) and system-include suite results (98/99 pass).
6. Updated version number and committed all changes.
7. Confirmed self-host C0→C1→C2 cycle with all tests passing.
