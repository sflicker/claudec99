# stage-84 Transcript: Standard Streams

## Summary

Stage 84 adds support for the standard C stream pointers `stdin`, `stdout`, and `stderr` to the compiler's controlled `stdio.h` stub header. These are extern-declared FILE* global variables that programs reference directly when using functions like `fprintf`. The implementation required enhancements to the code generator to properly handle extern-only object declarations: previously the codegen completely skipped such declarations in the first pass; now extern globals are tracked with an `is_extern` flag, registered in the globals table for the linker to resolve, and explicitly emitted as NASM `extern` directives during code generation.

## Changes Made

### Step 1: Header Updates

- Added `extern FILE *stdin;` to `test/include/stdio.h`
- Added `extern FILE *stdout;` to `test/include/stdio.h`
- Added `extern FILE *stderr;` to `test/include/stdio.h`

### Step 2: Codegen Data Structure

- Added `is_extern` boolean field to the `GlobalVar` struct in `include/codegen.h` to track extern-only global variables

### Step 3: Codegen First Pass

- Modified `codegen_add_global` in `src/codegen.c` to initialize `is_extern` based on whether the global declaration has the extern storage class specifier

### Step 4: Codegen Storage Emission

- Updated `codegen_emit_bss` to skip any global variables with `is_extern=1` (no BSS storage for extern globals)
- Updated `codegen_emit_data` to skip any global variables with `is_extern=1` (no data segment storage for extern globals)

### Step 5: Codegen Extern Emission

- Modified `codegen_emit_externs` to iterate over all globals and emit NASM `extern <name>` directives for any variable with `is_extern=1`, allowing the linker to resolve these external symbols

### Step 6: Version

- Updated `VERSION_STAGE` in `src/version.c` to "00840000"

### Step 7: Tests

- Created `test/valid/test_stdio_streams__1.c` to verify that the three stream pointers can be referenced and compared against null

## Final Results

All 1260 tests pass (1259 existing + 1 new). No regressions. The compiler now correctly handles extern-declared file-scope object variables and emits the appropriate NASM directives for the linker to resolve them.

## Session Flow

1. Read the stage 84 spec and related documentation
2. Reviewed the changes summary and identified affected files
3. Examined the codegen data structures and the flow for global variable handling
4. Added the `is_extern` field to `GlobalVar` struct and initialized it during global registration
5. Modified BSS and data emission functions to skip extern globals
6. Enhanced `codegen_emit_externs` to emit NASM extern directives for extern globals
7. Updated the version constant
8. Verified that the new test compiled and ran correctly
9. Ran the full test suite and confirmed 1260/1260 tests pass
