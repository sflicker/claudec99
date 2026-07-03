# stage-174 Transcript: Make --sysinclude Default; Add --testinclude

## Summary

Stage 174 is a pure frontend change to `bin/cc99` that flips the default include-path behavior on Linux x86_64 systems. Previously, `cc99` defaulted to using project stub headers in `test/include/` unless the user explicitly passed `--sysinclude` (which switched to real `/usr/include` headers). This stage inverts that default: on Linux x86_64, real system headers are now the default, and a new `--testinclude` flag allows users to explicitly request the old stub-header behavior. Platforms other than Linux x86_64 retain the original behavior (stub headers by default), since real system headers are not universally available or supported across all platforms. The implementation uses a tri-state `include_mode` variable (empty string / `sysinclude` / `testinclude`) to track the selection mode, with platform-specific default assignment after argument parsing.

## Changes Made

### Step 1: bin/cc99 Frontend Script Refactoring

- Replaced the boolean `use_sysinclude` variable with a tri-state `include_mode` variable holding values: empty string (unset), `"sysinclude"`, or `"testinclude"`.
- Added a new `--testinclude` command-line option case that sets `include_mode="testinclude"` when passed.
- Implemented post-argument-parsing default logic: if `include_mode` is still unset (empty string), assign it based on platform using the same Linux x86_64 detection (`uname -s` / `uname -m` check) that gates the explicit `--sysinclude` flag. On Linux x86_64, default to `"sysinclude"`; otherwise default to `"testinclude"`.
- Updated the active-include-flags selection logic in the compile loop to branch on `include_mode` value instead of the old boolean flag, setting `cc_include_flags` to either `-I/usr/include` or `-Itest/include` based on the mode.
- Updated `--help` text to document the new `--testinclude` option and note that `--sysinclude` is now the default on Linux x86_64 systems.

### Step 2: Test Investigation and Planning

- Investigated which test fixtures actually invoke `bin/cc99` directly (vs. calling the compiled `build/ccompiler` with explicit `-I` flags). Found that only `test/build_tool/test_make_hello` and `test/build_tool/test_make_cflags` use `cc99` in their workflows.
- Confirmed that all other test suites (portable, system-include, optional-library, and most of build-tool) bypass cc99's default-selection logic by passing explicit `-I` flags directly to the compiler, so they are unaffected by the default-flip change.
- Identified need for two new tests to directly exercise cc99's include-path selection logic: one for the new default (system headers without a flag), one for the explicit `--testinclude` option.

### Step 3: New Test Fixtures

- Created `test/build_tool/test_cc99_default_headers`: a simple C program that uses `#ifdef CLAUDEC99_STDIO_H` to detect which header set was included (the stub header's `test/include/stdio.h` defines this unique macro; real system headers never do). Run without any explicit include flag, expecting the default (sysinclude on Linux x86_64) to select real system headers, causing the ifdef to fail and the program to print "system" to stdout. The `.expected` file validates that output.
- Created `test/build_tool/test_cc99_testinclude`: identical logic and ifdef detection, but invoked with `--testinclude` flag, expecting the stub headers to be selected, causing the ifdef to succeed and printing "stub" to stdout, validated against its `.expected` file.
- Both tests are portable in structure but only run successfully on Linux x86_64 (where real system headers are available); on other platforms they would default to stub headers regardless of flag, making the test results platform-dependent but not stage-174-dependent.

### Step 4: Incidental Pre-Existing Issue Fix

- Discovered and removed a stale, gitignored CMake build cache file (`test/build_tool/test_cmake_hello/build/CMakeCache.txt`) that referenced an old, no-longer-valid local path. This file was leftover from a prior local cmake build and was causing the `test_cmake_hello` fixture to fail even before stage-174 changes. Removal resolved the failure and is not part of the stage 174 feature work, but necessary for clean test runs.

### Step 5: Documentation and Version Updates

- Updated `README.md`: Modified the `bin/cc99` options table to add a new `--testinclude` row documenting the flag's purpose; updated the `--sysinclude` row to note it is now the default on Linux x86_64.
- Updated `docs/outlines/checklist.md`: Added a new `## Stage 174 - Make --sysinclude the Default; Add --testinclude` section after the Stage 173 section.
- Incremented `src/version.c` `VERSION_STAGE` from `"01730000"` to `"01740000"`.
- Updated `docs/self-compilation-report.md` to prepend new "Issues found during stage 174 self-hosting test" and "Result (stage 174)" sections above the stage 173 entries.

## Final Results

Full test suite via `./test/run_all_tests.sh`: all green.
- Portable: 2074 passed, 0 failed (165 unit, 1286 valid, 261 invalid, 191 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- System-include: 191 passed, 0 failed.
- Optional-library: 2 passed, 0 failed, 0 skipped.
- Build-tool: 5 passed, 0 failed, 0 skipped (up from 3 pre-stage-174; the two new cc99-include-mode tests).

Manual smoke tests of `bin/cc99` confirmed all three modes function correctly:
- No flag (default on Linux x86_64): selects real system headers ("system" output).
- `--testinclude`: selects project stub headers ("stub" output).
- `--sysinclude`: selects real system headers (explicit, matches new default).

Self-host bootstrap via `./build.sh --mode=self-host` completed the full C0→C1→C2 cycle with no source changes required. All test suites passed at every stage (C0, C1, C2), each reporting version `00.03.01740000.0127x`. Since `bin/cc99` is a shell script and not part of the compiler's own bootstrap, the version bump and include-mode changes do not affect the self-compilation process—the compiler compiles itself identically. No regressions detected.

## Session Flow

1. Read the stage 174 spec and kickoff documentation to understand the feature requirements and design decisions.
2. Reviewed the current `bin/cc99` script logic to understand the existing `use_sysinclude` boolean and include-flags selection.
3. Investigated which test fixtures invoke `bin/cc99` directly vs. using explicit `-I` flags, confirming minimal impact scope.
4. Designed the tri-state `include_mode` implementation and platform-based default logic, reusing the existing Linux x86_64 detection gate for consistency.
5. Implemented the `bin/cc99` changes: tri-state variable, `--testinclude` option, post-argument default assignment, and compile-loop branching.
6. Updated `--help` text to document the new option and default behavior.
7. Designed and created two new `test/build_tool/` tests to directly exercise the include-path selection logic using the `CLAUDEC99_STDIO_H` macro detection method.
8. Discovered and fixed a stale CMake build cache in `test/build_tool/test_cmake_hello/build/`, resolving a pre-existing test failure.
9. Updated documentation: `README.md` options table, `docs/outlines/checklist.md` with new Stage 174 section, `src/version.c` version bump, and `docs/self-compilation-report.md` with stage-174 results.
10. Ran full test suite: all 2272 tests pass (2074 portable + 191 system-include + 2 optional-library + 5 build-tool), no regressions.
11. Manually tested `bin/cc99` in all three modes (default, `--testinclude`, `--sysinclude`) to confirm correct header-set selection.
12. Executed `./build.sh --mode=self-host` for full C0→C1→C2 bootstrap verification; all stages pass with zero source changes.
13. Generated milestone summary, transcript, and README update artifacts.

## Notes

### Design Decisions

1. **Platform Gate Consistency:** The default-selection logic reuses the existing Linux x86_64 check (not just "Linux") for consistency with the explicit `--sysinclude` gate and because the hardcoded `/usr/include/` system paths are x86_64-specific. On non-Linux or non-x86_64 platforms, the default falls back to testinclude (stub headers), preserving the original behavior and avoiding path-not-found errors.

2. **Flag Precedence:** If a user passes both `--sysinclude` and `--testinclude` on the command line, the last flag wins (standard CLI convention). No error is thrown for conflicting flags.

3. **Test Isolation:** The two new cc99-include-mode tests use a portable technique (the `CLAUDEC99_STDIO_H` include-guard macro detection) to validate which header set was selected, without hardcoding platform checks into the test logic. Both tests are designed to work on any platform, though the practical validation only occurs on Linux x86_64 where both header sets coexist.

### Incidental Fix

The stale CMake build cache (`test/build_tool/test_cmake_hello/build/CMakeCache.txt`) was a pre-existing local artifact leftover from a prior cmake build with an old path reference. Its removal was necessary for clean test runs but is not part of the stage 174 feature scope. This file is gitignored and not tracked in the repository, so its removal affects only the local test environment.
