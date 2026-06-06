# stage-94 Transcript: Self-Hosting Validation and Bootstrap Cycle

## Summary

Stage 94 is a process/validation stage with no new compiler language features. Its purpose was to validate the updated implement-stage skill's 4-phase workflow (analysis/planning, implementation, self-hosting test, documentation) and establish a robust, repeatable self-hosting bootstrap cycle. The stage focused entirely on build infrastructure improvements and documentation updates discovered during a comprehensive self-hosting test run. Five critical issues were identified and fixed: missing system header include path in bootstrap mode, missing version build number computation and passing, missing commit checkpoints to distinguish build versions, and an incomplete regex for capturing the build number in the BuiltBy token. A new `--mode=self-host` was added to `build.sh` to perform a complete and reproducible self-hosting cycle from fresh cmake build through two levels of bootstrapping. All test suites pass at each stage (C0, C1, C2) with full provenance chain verification.

## Changes Made

### Step 1: Bootstrap Include Path Fix

- Added `-I "$SCRIPT_DIR/test/include"` to the compiler invocation in `do_bootstrap_build` function in `build.sh`.
- This ensures bootstrap mode has access to stub system headers (stdio.h, stdlib.h, etc.) located in `test/include/`.
- Root cause: bootstrap only passed `-I include` (project headers), not the system header stubs needed by the compiler.

### Step 2: Version Build Number Computation

- Added `build_num=$(git rev-list --count HEAD)` to compute the build number in `build.sh`.
- Modified the compiler invocation to pass `-DVERSION_BUILD=${build_num}` for every bootstrap compilation.
- Root cause: `build.sh` never computed or passed `-DVERSION_BUILD`; cmake derives it automatically but bootstrap did not.

### Step 3: C0-to-C1 Checkpoint Commit

- Added `git commit --allow-empty` after C0 verification completes in the new `--mode=self-host` workflow.
- This ensures C1 (bootstrapped from C0) always has a different build number than C0.
- Root cause: no commit occurred between C0's cmake build and C1's bootstrap, so C1 had identical version string as C0.

### Step 4: C1-to-C2 Checkpoint Commit

- Added `git commit --allow-empty` after C1 verification completes in `--mode=self-host`.
- This ensures C2 (bootstrapped from C1) always has a different build number than C1.
- Root cause: no commit occurred between C1's bootstrap and C2's bootstrap, so C2 had identical version string as C1.

### Step 5: BuiltBy Regex Fix

- Updated the regex pattern in `do_bootstrap_build` from `[0-9]+\.[0-9]+\.[0-9]+` to `[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+`.
- This captures all four version groups (major.minor.stage.build) instead of only three, preserving the build number in the BuiltBy token.
- Root cause: original regex matched only 3 of 4 version groups, discarding the build number (e.g., `ClaudeC99_v00_02_00940000` instead of `ClaudeC99_v00_02_00940000_00657`).

### Step 6: New --mode=self-host Workflow

- Added `--mode=self-host` to `build.sh` with the following workflow:
  1. Archives existing `build/ccompiler`, `build/ccompiler-c0/c1/c2` to `build/previous/` for safety.
  2. Performs a fresh cmake build (always with system GCC) and saves as `build/ccompiler-c0`.
  3. Runs the full test suite with C0; commits C0 checkpoint with `git commit --allow-empty`.
  4. Bootstraps C0 to produce C1; runs full test suite; commits C1 checkpoint.
  5. Bootstraps C1 to produce C2; runs full test suite.
  6. Reports final C0/C1/C2 version strings and BuiltBy tokens; leaves `build/ccompiler` as C2.
- The mode is fully self-contained and idempotent: running it twice produces identical results.
- Designed to be the authoritative self-hosting validation method in the implement-stage skill.

### Step 7: Documentation and Skill Updates

- Updated `src/version.c` with `VERSION_STAGE` set to `"00940000"` (stage 94, build 0000).
- Updated `docs/self-compilation-report.md` with a comprehensive table of all 5 issues, root causes, and fixes.
- Updated `docs/self-compilation-report.md` with the final self-host test result table showing C0/C1/C2 versions and BuiltBy tokens.
- Expanded README.md Build section with a table showing build.sh modes: normal, bootstrap, self-host, fallback.
- Updated README.md "Through stage N" marker to "Through stage 94".
- Updated `.claude/skills/implement-stage/SKILL.md` Self Host Test Phase to reference `--mode=self-host` as the canonical method.

## Final Results

**Build System:** New `--mode=self-host` establishes a complete and repeatable self-hosting validation cycle. All five critical issues were identified and fixed.

**Issue Summary:**

| # | Issue | Root Cause | Fix |
|---|-------|------------|-----|
| 1 | `build.sh --mode=bootstrap` failed with `error: include file not found: <stdio.h>` | Bootstrap only passed `-I include`, not `-I test/include` | Added `-I "$SCRIPT_DIR/test/include"` to bootstrap invocation |
| 2 | C1 and C2 showed `00000` as build number | `build.sh` never computed or passed `-DVERSION_BUILD` | Added `git rev-list --count HEAD` and `-DVERSION_BUILD=${build_num}` |
| 3 | C1 and C2 had identical version strings | No commit between C1 and C2 bootstrap runs | Added `git commit --allow-empty` checkpoint after C1 verified |
| 4 | C0 and C1 had identical version strings | No commit between cmake build and first bootstrap | Added `git commit --allow-empty` checkpoint after C0 verified |
| 5 | BuiltBy omitted build number (e.g., `ClaudeC99_v00_02_00940000` instead of `ClaudeC99_v00_02_00940000_00657`) | Regex matched only 3 of 4 version groups | Changed regex to `[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+` |

**Final Self-Host Test Results:**

| Binary | Version | BuiltBy | Tests |
|--------|---------|---------|-------|
| `build/ccompiler-c0` | `00.02.00940000.00657` | `GNU_13_3_0` | 1306/1306 |
| `build/ccompiler-c1` | `00.02.00940000.00658` | `ClaudeC99_v00_02_00940000_00657` | 1306/1306 |
| `build/ccompiler-c2` | `00.02.00940000.00659` | `ClaudeC99_v00_02_00940000_00658` | 1306/1306 |

**Provenance Chain:** C1's BuiltBy names C0's exact version (`00.02.00940000.00657`); C2's BuiltBy names C1's exact version (`00.02.00940000.00658`). Full traceability from system compiler through two bootstrap generations established.

**Test Status:** All 1306 tests pass in all three compilers (C0 gcc-built, C1 bootstrapped from C0, C2 bootstrapped from C1). 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm. No regressions.

## Session Flow

1. Read the stage spec identifying the purpose: process/validation stage with no language features, goal to validate 4-phase workflow and establish repeatable self-hosting cycle.
2. Reviewed the self-compilation-report.md to identify issues encountered during initial self-hosting validation.
3. Analyzed all five issues: include path, version build number, checkpoint commits, and BuiltBy regex.
4. Implemented fixes in `build.sh`: added system header include path, computed and passed VERSION_BUILD, added commit checkpoints, fixed regex.
5. Created `--mode=self-host` workflow integrating all fixes with a complete self-hosting validation cycle.
6. Updated `src/version.c` to VERSION_STAGE "00940000".
7. Tested the new self-host mode end-to-end: C0 cmake build, C1 bootstrap, C2 bootstrap, all test suites passing.
8. Verified provenance chain: each BuiltBy token correctly names the exact version that built it.
9. Updated documentation: self-compilation-report.md, README.md, and implement-stage SKILL.md.
10. Confirmed all changes are build infrastructure and documentation; no compiler language features added.
