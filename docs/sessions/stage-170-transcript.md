# stage-170 Transcript: Warning Level Flags (-Wall, -Wextra)

## Summary

Stage 170 adds warning-level flag support (`-Wall` and `-Wextra`) to the compiler as pure infrastructure. Both flags are parsed, validated, and forwarded through the compilation pipeline without emitting any new diagnostic checks. The implementation introduces a global `g_warn_level` integer that tracks the active warning level (0 = none, 1 = -Wall, 2 = -Wall -Wextra), mirroring the approach taken in Stage 142 for optimization flags (`-O0` / `-O1`). The infrastructure plumbing is complete; per-diagnostic warning implementations are deferred to follow-up stages. All existing tests continue to pass, and output assembly is identical regardless of warning flags since no actual checking logic is wired in yet.

## Changes Made

### Step 1: Global Warning Level Variable

- Added `extern int g_warn_level;` declaration in `include/util.h` after the `g_warnings_are_errors` declaration.
- Added `int g_warn_level = 0;` initializer in `src/util.c` after the `g_warnings_are_errors` initializer.
- The global is initialized to 0 (no warning groups active) and is visible to all compilation phases without threading through call chains.

### Step 2: Compiler Flag Parsing

- Added `int warn_level = 0;` local variable in `src/compiler.c` main() function alongside `warnings_are_errors`.
- Added two new argument branches in the main loop: `-Wall)` sets `warn_level = 1` if not already higher, `-Wextra)` sets `warn_level = 2` (always); both branches immediately assign `g_warn_level = warn_level` to make the level visible globally.
- Updated the usage string to include `[-Wall] [-Wextra]` after `[-Werror]`.
- No changes to `compile_one_file()` signature; `g_warn_level` is a global and reaches all code automatically.

### Step 3: Wrapper Script Enhancement

- Added `-Wall|-Wextra)` case in `bin/cc99` argument parser that accumulates both flags into `compiler_flags` array (same pattern as `-Werror`).
- Updated the usage block in `bin/cc99` to document both flags with brief descriptions: `-Wall` enables common warnings, `-Wextra` enables additional warnings and implies `-Wall`.
- Both flags are forwarded to every `ccompiler` invocation unchanged.

### Step 4: Checklist Update

- Updated `docs/outlines/checklist.md` to mark "Warning level support (-Wall, -Wextra)" as complete.
- Added 8 indented sub-items for follow-up stages: `-Wunused-variable`, `-Wunused-function`, `-Wunused-parameter`, `-Wimplicit-function-declaration`, `-Wreturn-type`, `-Wsign-compare`, `-Wmissing-field-initializers`, and `-Wformat` (future).

### Step 5: Version Update

- Incremented `VERSION_STAGE` in `src/version.c` from `01690000` to `01700000`.

## Final Results

All 2072 portable tests pass (165 unit, 1286 valid, 261 invalid, 189 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 189 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage. No source changes needed during bootstrap. C0 version 00.03.01700000.01249, C1 version 00.03.01700000.01250, C2 version 00.03.01700000.01251. Output assembly is identical when compiled with or without `-Wall` / `-Wextra` since no diagnostic checks are active yet.

## Session Flow

1. Read specification and reviewed the Stage 142 optimizer flag precedent.
2. Examined the existing `-Werror` infrastructure in `include/util.h` and `src/util.c`.
3. Added `g_warn_level` global declaration and initialization.
4. Added `warn_level` local variable and argument parsing in `src/compiler.c`.
5. Updated `ccompiler` usage string to include both new flags.
6. Added argument parsing in `bin/cc99` to forward both flags.
7. Updated `bin/cc99` usage block with flag documentation.
8. Updated checklist with completion mark and per-diagnostic sub-items.
9. Bumped version number.
10. Ran full test suite and verified flag acceptance produces correct output.
11. Verified self-host bootstrap C0→C1→C2 with all tests passing at every stage.
