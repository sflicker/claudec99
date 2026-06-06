# stage-93 Transcript: Bootstrap Build Workflow

## Summary

Stage 93 implements a multi-mode build system to support normal (gcc/clang), bootstrap (self-hosted), and fallback build strategies. A new `build.sh` wrapper script with three command-line modes allows flexible build configuration. The bootstrap mode uses a pre-built `build/ccompiler` to compile each source file to NASM assembly, then assembles and links with nasm and gcc. Timeout guards (300s per file by default, configurable) prevent infinite loops. The version system was enhanced to record which compiler was used to build each binary via a new `VERSION_BUILTBY` macro, stringified via C99's `#` operator. All test suites were augmented with timeout variables (default 30s) to prevent runaway test execution. No compiler language features were changed; this is a build infrastructure and bootstrap-safety stage. All 1306 tests pass.

## Changes Made

### Step 1: Version System — BUILD_BY Attribution

- Updated `src/version.c` to add `VERSION_BUILTBY` macro support via C99 stringification.
- Added a string-helper macro pair (`CC99_STR_HELPER` and `CC99_STR`) to convert the compiler token into a C string literal at compile time.
- Default value is the token `DefaultCcompiler` (stringified to `"DefaultCcompiler"`).
- Extended version output from one line to two lines: `ClaudeC99 version <full_version>\nBuiltBy: <compiler_token>`.
- Expanded `version_buf` from 128 bytes to 256 bytes to accommodate the longer output.
- Updated `VERSION_STAGE` to `"00930000"`.

### Step 2: CMake Build System — BUILTBY_TOKEN Computation

- Modified `CMakeLists.txt` to compute `BUILTBY_TOKEN` from the C compiler's identity and version.
- Extracts `CMAKE_C_COMPILER_ID` (e.g., `GNU`, `Clang`, `MSVC`) and `CMAKE_C_COMPILER_VERSION` (e.g., `13.3.0`).
- Replaces dots in the version with underscores to produce a valid C identifier token (e.g., `GNU_13_3_0`).
- Passes the computed token via `-DVERSION_BUILTBY=<token>` to the compiler for all translation units.

### Step 3: Bootstrap Build Wrapper Script

- Created new `build.sh` script with three operation modes specified via `--mode=<mode>`:
  - `--mode=normal` (default): Runs standard cmake build with `cmake -S . -B build` and `cmake --build build`.
  - `--mode=bootstrap`: Assumes pre-built `build/ccompiler` exists. For each source file in `src/`, invokes ccompiler to generate NASM assembly, then assembles all `.asm` files with `nasm -f elf64`, and links with `gcc` to produce `build/ccompiler`. Includes `--timeout=<seconds>` option (default 300) per-file compilation timeout to guard against infinite loops during compilation.
  - `--mode=fallback`: Uses bootstrap mode if `build/ccompiler` exists; otherwise falls back to normal mode.
- Bootstrap mode passes `VERSION_BUILD` and `VERSION_BUILTBY=SelfHosted` (when bootstrapping with the newly built compiler).

### Step 4: Test Suite Timeout Guards — Valid Tests

- Updated `test/valid/run_tests.sh` to add `TIMEOUT=${CLAUDEC99_TEST_TIMEOUT:-30}` variable.
- Wrapped compiler invocation with `timeout "$TIMEOUT"` to prevent runaway compilation.
- Wrapped program execution with `timeout "$TIMEOUT"` to prevent infinite loops in generated code.

### Step 5: Test Suite Timeout Guards — Invalid Tests

- Updated `test/invalid/run_tests.sh` to add `TIMEOUT` variable and wrapped compiler invocation with `timeout "$TIMEOUT"`.

### Step 6: Test Suite Timeout Guards — Integration Tests

- Updated `test/integration/run_tests.sh` to add `TIMEOUT` variable.
- Wrapped both error-test path and normal compilation path with `timeout "$TIMEOUT"`.
- Wrapped program execution with `timeout "$TIMEOUT"`.

### Step 7: Test Suite Timeout Guards — Print AST

- Updated `test/print_ast/run_tests.sh` to add `TIMEOUT` variable and wrapped compiler invocation with `timeout "$TIMEOUT"`.

### Step 8: Test Suite Timeout Guards — Print Tokens

- Updated `test/print_tokens/run_tests.sh` to add `TIMEOUT` variable and wrapped compiler invocation with `timeout "$TIMEOUT"`.

### Step 9: Test Suite Timeout Guards — Print ASM

- Updated `test/print_asm/run_tests.sh` to add `TIMEOUT` variable and wrapped compiler invocation with `timeout "$TIMEOUT"`.

## Final Results

**Build System:** Three build modes now available via `build.sh --mode={normal|bootstrap|fallback}` with configurable per-file timeout (default 300s) for bootstrap. CMake integration computes and passes `VERSION_BUILTBY` macro. Normal build path unchanged; bootstrap tested successfully.

**Version Output:** Example output format:
```
ClaudeC99 version 00.02.00930000.00641
BuiltBy: GNU_13_3_0
```

**Test Status:** All 1306 tests pass (823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). No test additions (infrastructure stage only). No regressions. All suites now resilient to timeout-protected execution.

**Bootstrap Safety:** Per-file and per-test timeouts (300s and 30s defaults) prevent runaway compilation and execution loops.

## Session Flow

1. Read the spec and identified required changes: build modes, version attribution, and timeout guards.
2. Reviewed spec issues and decided on fixes: proper macro stringification, token naming convention, correct version field name.
3. Implemented Version system updates: added stringification macros, extended version output to two lines, expanded buffer.
4. Modified CMakeLists.txt to compute and pass BUILTBY_TOKEN based on compiler ID and version.
5. Created build.sh with three modes: normal, bootstrap, and fallback.
6. Added timeout support to bootstrap mode with `--timeout` flag (300s default).
7. Updated all six test suite runners to add TIMEOUT variables and wrap invocations.
8. Tested normal build path (all 1306 tests pass).
9. Verified bootstrap mode with pre-built compiler and timeout enforcement.
10. Documented changes and confirmed no language feature changes or test regressions.
