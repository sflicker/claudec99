# stage-169 Transcript: Debug Information (DWARF)

## Summary

Stage 169 adds DWARF debug information support via a new `-g` compiler flag. When the compiler is invoked with `-g`, it emits NASM `%line N+0 "file.c"` directives into the generated assembly before each function label and at key statement execution points. These directives tell NASM to produce `.debug_line`, `.debug_abbrev`, and `.debug_info` sections in the ELF object file. GDB can then read these sections to map machine instructions back to their source lines, enabling breakpoints, step-through debugging, and correct stack traces with file and line information.

The implementation is minimal and non-invasive: the debug flag is passed through the compilation pipeline without affecting code generation logic, and debug line directives are emitted as side-effects of the existing codegen walk. The `bin/cc99` wrapper script and the integration test runner both detect the `-g` flag in compiler arguments and automatically pass `-g -F dwarf` to NASM when assembling debug-instrumented code.

## Changes Made

### Step 1: Codegen Structure and Initialization

- Added three fields to the `CodeGen` struct in `include/codegen.h`: `emit_debug` (int flag), `debug_last_file` (const char * for deduplication), `debug_last_line` (int for deduplication).
- Modified `codegen_init()` in `src/codegen.c` to zero-initialize all three new fields.
- Added static helper `emit_debug_line(CodeGen *cg, const char *file, int line)` in `src/codegen.c` that guards on `cg->emit_debug`, emits `%line N+0 "file"` only if the file or line changed since the last emission (deduplication), and updates `debug_last_file` and `debug_last_line` to track the last emitted position.

### Step 2: Codegen Integration

- Modified `codegen_function()` in `src/codegen.c` to call `emit_debug_line()` before emitting the function label, passing the source node's file and line.
- Modified `codegen_statement()` in `src/codegen.c` to call `emit_debug_line()` after `cg_mark(node)` for each statement, passing the statement node's source location.
- These calls ensure that GDB can map the entry point of each function and the execution of each statement to their source locations.

### Step 3: Compiler Flag Parsing

- Added `emit_debug` local variable to `compile_main()` in `src/compiler.c`, initialized to 0.
- Added `-g)` case branch in the argument parser that sets `emit_debug = 1`.
- Modified `compile_one_file()` function signature to accept an `int emit_debug` parameter.
- Added `cg.emit_debug = emit_debug;` assignment before calling `codegen_translation_unit()`.
- Updated the usage string to include `[-g]` in the option list.

### Step 4: Wrapper Script Enhancement

- Modified `bin/cc99` to add a `-g)` case in the argument parser that marks the `-g` flag as recognized (alongside `-O0`, `-O1`, `-O2`).
- Added post-loop detection of the `-g` flag by scanning `compiler_flags` array for the string `-g`.
- When debug is enabled, passed `nasm_debug_flags=(-g -F dwarf)` to NASM via the `nasm_cmd` assembly invocation.

### Step 5: Test Runner Integration

- Modified `test/integration/run_tests.sh` to detect the presence of `-g` in the `compiler_flags` companion file.
- When debug is detected, sets `nasm_debug_flags=(-g -F dwarf)` and passes these flags to NASM when assembling each test's output.

### Step 6: New Integration Test

- Created `test/integration/test_dwarf_debug/test_dwarf_debug.c` with a simple test program: static helper functions `add(int, int)` and `mul(int, int)` that return 3+4=7 and 6*1=6, respectively; `main()` computes `7*6=42` and returns it.
- Created `test/integration/test_dwarf_debug/test_dwarf_debug.cflags` containing `-g` to enable debug info emission.
- Created `test/integration/test_dwarf_debug/test_dwarf_debug.status` containing `42` as the expected exit code.

### Step 7: Version Update

- Incremented `VERSION_STAGE` in `src/version.c` from `01680000` to `01690000`.

## Final Results

All 2072 portable tests pass (165 unit, 1286 valid, 261 invalid, 189 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 189 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage. No source changes needed during bootstrap. C0 version 00.03.01690000.01243, C1 version 00.03.01690000.01244, C2 version 00.03.01690000.01245.

## Session Flow

1. Read specification and reviewed existing DWARF debug info scaffolding.
2. Examined current code generation pipeline to identify integration points.
3. Added `emit_debug` fields to `CodeGen` struct and initialized them in `codegen_init()`.
4. Implemented `emit_debug_line()` helper with deduplication logic.
5. Wired helper calls into `codegen_function()` and `codegen_statement()`.
6. Added `-g` flag parsing to `src/compiler.c` and propagated `emit_debug` parameter.
7. Updated `bin/cc99` wrapper to detect `-g` and pass NASM debug flags.
8. Updated `test/integration/run_tests.sh` to handle `-g` in cflags.
9. Created new integration test with debug symbols enabled.
10. Bumped version number and verified full test suite and self-hosting.
