# stage-155 Transcript: Peephole Optimizer Infrastructure

## Summary

Stage 155 establishes the post-codegen peephole optimizer infrastructure. The peephole pass operates on the final NASM assembly output as an array of lines, scanning forward with a sliding window of 2–4 consecutive lines and applying registered pattern-matcher and replacer function pairs. This stage adds the infrastructure only: type definitions for matchers and replacers, the forward-scanning window engine, and file I/O machinery. No patterns are registered, so the pass is a transparent no-op at `-O2`. Future stages will implement individual optimization patterns (zero-register idiom, move elimination, push/pop collapse, etc.) by registering them in the pattern table.

The peephole pass is part of Optimize Level 2 (`-O2`), which is a superset of Level 1 (`-O1`): when `opt_level >= 2`, both the AST-level optimizer and the peephole pass run. The peephole layer keeps codegen clean—a pure AST-to-NASM translation—while handling machine-level cleanup separately.

## Changes Made

### Step 1: Header Definition (peephole.h)

- Created `include/peephole.h` with `PEEPHOLE_WINDOW_MAX` macro (value 4).
- Defined `PeepholeMatcher` function pointer type: takes a read-only array of `n` trimmed strings and returns 1 if matched, 0 otherwise.
- Defined `PeepholeReplacer` function pointer type: takes the matched window, fills a caller-provided output array with replacement lines (heap-allocated), and sets the count (0 means delete all).
- Defined `PeepholePattern` struct: window_size (1 to PEEPHOLE_WINDOW_MAX), matcher function pointer, replacer function pointer.
- Declared public function `peephole_apply`: runs patterns over a mutable line array in place; patterns tried in order; first match fires; scanning resumes after replacement (no re-scan).
- Declared public function `peephole_run_file`: read file into line array, apply patterns, write result back; returns 0 on success, 1 on I/O failure.

### Step 2: Core Implementation (peephole.c)

- Created `src/peephole.c` with file-top comment referencing Stage 155 infrastructure.
- Implemented `read_lines` helper: opens file, reads lines into heap-allocated array (one string per line, no trailing newlines), handles memory allocation and growth, strips CR/LF, returns 0 on success or 1 on failure.
- Implemented `write_lines` helper: opens file for writing, writes each line followed by newline, returns 0 on success or 1 on failure.
- Implemented `peephole_apply` sliding window engine: iterates forward through line array; for each position, tries all registered patterns in order; on match, replacer fills output buffer, rebuilds array (prefix + replacements + suffix), frees old window lines, installs replacement lines, advances index past replacements (no re-scan); advances by one line if no match.
- Implemented `peephole_run_file` orchestrator: calls `read_lines`, calls `peephole_apply`, calls `write_lines`, frees memory, returns status.
- Included headers: `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `"util.h"`, `"peephole.h"`.

### Step 3: Build System

- Updated `CMakeLists.txt`: added `src/peephole.c` to source list in `add_executable` (after `src/optimize.c`).
- Updated `build.sh`: added `src/peephole.c` to `SRC_FILES` array (required for bootstrap self-hosting).

### Step 4: Compiler Integration

- Updated `src/compiler.c`: added `#include "peephole.h"` to include block.
- Added `-O2` flag parsing in argument-parsing loop in `main`.
- Updated usage string to document `-O2` option.
- Added peephole invocation in `compile_one_file`: after `fclose(out)`, call `peephole_run_file(output_path, NULL, 0)` when `opt_level >= 2`. At this stage patterns array is NULL with count 0, so pass reads file, makes no changes, and writes back unchanged.

### Step 5: Documentation Updates

- Updated `src/optimize.c` file-top comment to reference Stage 155 peephole infrastructure.
- Updated `src/version.c`: bumped `VERSION_STAGE` to `"01550000"`.
- Updated `docs/outlines/checklist.md`: marked peephole infrastructure item as complete; added `## Stage 155` section after Stage 154.

### Step 6: Integration Tests

- Created `test/integration/test_peephole_noop/test_peephole_noop.c`: simple program (a=1, b=2, print a+b); `.cflags` file: `-O2`; `.expected`: `3`.
- Created `test/integration/test_peephole_noop_loop/test_peephole_noop_loop.c`: loop summing 1 to 5; `.cflags` file: `-O2`; `.expected`: `15`.
- Created `test/integration/test_peephole_noop_function/test_peephole_noop_function.c`: multiple functions (add, mul); `.cflags` file: `-O2`; `.expected`: `7 12`.

All three tests verify that `-O2` compiles correctly and produces expected output when no patterns are active.

### Step 7: Testing and Verification

- Built compiler: `cmake --build build`.
- Ran full test suite: 2045 portable tests pass (165 unit, 1286 valid, 261 invalid, 162 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- Ran self-host verification: C0→C1→C2 bootstrap chain passes all tests at every stage.
- Fixed two bootstrap issues discovered during self-host run:
  1. `const char * const *` parameter annotation in peephole.h not supported by C0 compiler; changed to `const char **`.
  2. `src/peephole.c` missing from `build.sh` SRC_FILES array; added to enable bootstrap compilation.

## Final Results

All 2045 portable tests pass (165 unit, 1286 valid, 261 invalid, 162 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
No regressions. Bootstrap chain C0→C1→C2 verified; all tests pass at every stage.

## Session Flow

1. Read spec and supporting files (milestone/transcript templates, Stage 155 spec, project layout).
2. Reviewed peephole.h specification and header design.
3. Implemented peephole.c: read_lines, write_lines, peephole_apply sliding window engine, peephole_run_file orchestrator.
4. Updated CMakeLists.txt and build.sh to include src/peephole.c.
5. Modified src/compiler.c: added -O2 flag parsing and peephole_run_file invocation after codegen.
6. Updated documentation in src/optimize.c and src/version.c.
7. Created three integration tests verifying -O2 correctness with no active patterns.
8. Built compiler and ran full test suite (2045/2045 pass).
9. Ran bootstrap self-host verification; discovered and fixed two issues (const char * const * annotation, build.sh SRC_FILES).
10. Verified all tests pass at C0→C1→C2 stages; stage complete.
