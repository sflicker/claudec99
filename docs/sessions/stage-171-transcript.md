# stage-171 Transcript: --help, Verbose Mode, and Mixed Input File Types

## Summary

Stage 171 adds three user-facing CLI improvements: `--help` flag support, verbose mode (`-v`/`--verbose`), and mixed input file type support in `bin/cc99`. The `--help` flag is implemented in both `ccompiler` and the `bin/cc99` wrapper script, printing formatted help to stdout and exiting cleanly. Verbose mode introduces a global `g_verbose` integer that gates the `compiled: X -> Y` progress line in `src/compiler.c`, allowing users to suppress progress noise in automated builds. The `bin/cc99` wrapper gains a `run_cmd` helper that prints each external tool invocation (ccompiler, nasm, gcc) to stderr when verbose is enabled. Most significantly, `bin/cc99` now accepts pre-assembled (`.s`/`.asm`), pre-compiled object (`.o`), and library (`.a`/`.so`) files alongside `.c` sources, routing each file type through the appropriate pipeline stage(s). The compilation loop was refactored with extension-based dispatch to determine whether the compile step is needed. All existing tests pass without modification, and self-hosted bootstrap (C0→C1→C2) verified successfully.

## Changes Made

### Step 1: Global Verbose Flag

- Added `extern int g_verbose;` declaration in `include/util.h` after `g_warn_level`.
- Added `int g_verbose = 0;` initializer in `src/util.c` after `g_warn_level`.
- Global is visible to all compilation phases without threading parameters.

### Step 2: Compiler --help and Verbose Support

- Added `--help` branch in `src/compiler.c` main() argument loop that prints formatted help message (usage line, option descriptions) to stdout and returns 0.
- Added `-v`/`--verbose` branch in main() argument loop that sets `verbose = 1` local variable and assigns `g_verbose = 1` global.
- Wrapped the `printf("compiled: %s -> %s\n", ...)` line in conditional `if (g_verbose)` guard so progress is only printed in verbose mode.
- Updated error-path usage string to recommend running `ccompiler --help` for more information.
- Both new branches integrate seamlessly with existing argument parsing; `g_verbose` is a simple global guard.

### Step 3: Wrapper Script --help and Verbose Support

- Added `--help` case in `bin/cc99` argument parser that prints formatted help block with usage line, input file type descriptions (`.c`, `.s`, `.asm`, `.o`, `.a`, `.so`), and all option documentation to stdout, then exits 0.
- Added `-v|--verbose` case that sets `verbose=1` local variable and appends `-v` to the `compiler_flags` array for forwarding to `ccompiler`.
- Both cases placed in the existing `case` argument block alongside other flag handlers.

### Step 4: run_cmd Helper and Tool Invocation Logging

- Added `run_cmd()` bash helper function in `bin/cc99` that conditionally logs the command to stderr when `verbose == 1`, then executes it.
- Logic: `[[ "$verbose" -eq 1 ]] && echo "cc99:" "$@" >&2` prints the full command with `cc99:` prefix to stderr before execution.
- Replaced all bare invocations (`"$COMPILER"`, `nasm`, `gcc`) with `run_cmd` equivalent, enabling verbose logging of the complete compilation pipeline.

### Step 5: Extended Input File Classification

- Modified the input file case block to accept five file types:
  - `*.c)` — C source files (already supported).
  - `*.s|*.asm)` — Assembly files (NASM syntax); added to `input_files` array.
  - `*.o)` — Object files; added to `input_files` array.
  - `*.a|*.so)` — Static/shared libraries; **added directly to `link_flags` array** (never enter compilation loop).
  - `-- ... *)` — Explicit end of options and unrecognized input error handling remain.
- `.a` and `.so` files bypass the compilation loop entirely because they are finalized library artifacts; they are passed to the final `gcc` link step as positional arguments.

### Step 6: Refactored Compilation Loop with Extension Dispatch

- Replaced the previous flat loop body with a new version that determines the compilation pipeline entry point from the input file's extension.
- For each input file in `input_files[@]`:
  1. Resolve the absolute path with `realpath`; fail if the file does not exist.
  2. **Dispatch on extension**: Set `needs_compile=1` for `.c` files, `needs_compile=0` for `.s`/`.asm` files, and `continue` (skip entirely) for `.o` files (unless in `-c` mode, in which case object files are passed directly to the link step).
  3. For `.s`/`.asm` files, copy the file into the temporary directory under the expected name (`${base}.asm`) so the assemble step can process it uniformly.
  4. **Compile phase** (`.c` only): Invoke `run_cmd "$COMPILER" ...` in the temp directory to produce `${base}.asm`.
  5. **Skip print/asm-only modes** as appropriate for non-compile sources.
  6. **Assemble phase**: Invoke `run_cmd nasm -f elf64 ...` to produce `${base}.o` (all sources that reached this point have a `.asm` file).
  7. **Link phase handling**: Append `${base}.o` to `obj_files` for later linking, or copy to output if in `-c` mode.
- This dispatch pattern cleanly separates compilation concerns and makes the pipeline transparent to mixed-source builds.

### Step 7: Link Step with run_cmd

- Replaced the bare `gcc` invocation with `run_cmd gcc ...` to enable logging when verbose.
- Receives `obj_files[@]` (accumulated object files) and `link_flags[@]` (library and library-path flags, plus `.a`/`.so` files added during argument parsing).

### Step 8: Version Update

- Incremented `VERSION_STAGE` in `src/version.c` from `01700000` to `01710000`.

## Final Results

All 2072 portable tests pass (165 unit, 1286 valid, 261 invalid, 189 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 189 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage. C0 version 00.03.01710000.01255, C1 version 00.03.01710000.01256, C2 version 00.03.01710000.01257. No source changes needed during bootstrap.

## Session Flow

1. Read specification and reviewed Stage 170 precedents for global flags and wrapper argument parsing.
2. Added `g_verbose` global declaration and initialization in `include/util.h` and `src/util.c`.
3. Implemented `--help` branch in `src/compiler.c` with formatted help output.
4. Implemented `-v`/`--verbose` branch in `src/compiler.c` to set the global flag.
5. Wrapped the progress line in `src/compiler.c` with conditional `if (g_verbose)`.
6. Added `--help` case in `bin/cc99` with comprehensive help block.
7. Added `-v|--verbose` case in `bin/cc99` to forward to `ccompiler`.
8. Implemented `run_cmd` helper to log external tool invocations when verbose.
9. Extended input file classification cases to accept `.s`/`.asm`/`.o`/`.a`/`.so`.
10. Refactored the compilation loop with extension dispatch to handle all input types.
11. Updated the gcc link invocation to use `run_cmd`.
12. Bumped version to `01710000`.
13. Ran full test suite (2072 portable, 189 system-include, 2 optional-library) — all pass.
14. Verified self-host C0→C1→C2 with no source changes.
