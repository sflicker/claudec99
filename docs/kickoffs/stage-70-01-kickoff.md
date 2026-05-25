# Stage 70.01 Kickoff: Add Additional Tooling Features

## Summary of the Spec

Stage 70.01 adds two tooling features to the ClaudeC99 compiler:

1. **Versioning System**: A new `version.h` and `version.c` module providing version information in the format `MM.mm.SSSSSSSS.BBBBB` where:
   - `MM` = major version (hardcoded as 00)
   - `mm` = minor version (hardcoded as 01)
   - `SSSSSSSS` = stage number (00700100 for stage 70.01, with format SSMMSSSS for main.sub.subsub)
   - `BBBBB` = build number (derived from git commit count, updated via cmake)
   - The `--version` flag displays "ClaudeC99 version MM.mm.SSSSSSSS.BBBBB" plus an optional extra info field (initially blank)
   - A `get_version_string()` function returns the full version string

2. **Error Limit Flag**: A new `--max-errors=N` flag controlling error collection:
   - When N=0: compiler continues through entire file collecting all errors (using setjmp/longjmp for error recovery)
   - When N>0: compiler stops after encountering N errors
   - Requires implementing `compile_error()` function with longjmp-based error recovery in parse_translation_unit
   - The cc99 script is updated to forward both `--version` and `--max-errors=N` flags to ccompiler

## Required Tokenizer Changes

None. No new tokens or lexical features are required.

## Required Parser Changes

1. Replace direct `fprintf()` + `exit()` error calls with `compile_error()` function calls throughout `src/parser.c`
2. Add error recovery architecture in `parse_translation_unit()`:
   - Wrap parsing logic with setjmp to establish error recovery point
   - When `compile_error()` is called with N < max_errors, longjmp returns to recovery point and continues
   - Track current error count in global variable accessible to `compile_error()`
3. Implement `parser_sync()` helper function for error synchronization (skipping tokens until safe recovery point)
4. The parser must permit continuing after encountering parse errors when error count hasn't reached limit

## Required AST Changes

None. No AST structure modifications are needed.

## Required Code Generation Changes

Replace direct `fprintf()` + `exit()` error calls in `src/codegen.c` with `compile_error()` function calls. Code generation errors are treated as fatal and don't participate in error recovery (compile_error on codegen errors should still exit immediately).

## Required File Modifications

### New Files

1. **include/version.h**: Header declaring `get_version_string()` function
2. **src/version.c**: Implementation containing hardcoded version components and `get_version_string()` function

### Modified Files

1. **include/util.h**: Add declarations for:
   - `compile_error()` function for error reporting with recovery support
   - Global variables for error tracking (error_count, max_errors)

2. **src/util.c**: Implement:
   - `compile_error(const char *format, ...)` function using setjmp/longjmp
   - Error recovery state management

3. **src/compiler.c**: Add flag parsing for:
   - `--version`: Display version and exit
   - `--max-errors=N`: Set maximum error limit (default: 1 for immediate exit on first error)

4. **CMakeLists.txt**:
   - Add `src/version.c` to build
   - Configure cmake to execute git command to get commit count
   - Pass commit count to version.c (or version generation mechanism)

5. **bin/cc99** (shell script): Update to forward `--version` and `--max-errors=N` flags to ccompiler executable

## Implementation Notes

- **Flag name decision**: Spec lists `--max-error` (singular), implementing as `--max-errors` (plural) to match standard convention
- **Build number**: Updated each time cmake configure step runs via `execute_process()` calling `git rev-list --count HEAD`
- **Error recovery**: Non-trivial architecture requiring setjmp/longjmp; errors are collected at parse level only, codegen errors remain fatal
- **Default behavior**: When max_errors not specified or set to 1, compiler stops on first error (existing behavior)
- **Version string format example**: "ClaudeC99 version 00.01.00700100.12345" (where 12345 is commit count)

## Test Plan

### Unit Tests (test/valid/ and test/invalid/)

**Version flag tests**
- `test_version_flag_output.c`: Compile with `--version` flag, verify output format and exit code
- Test version string contains all components: major.minor.stage.build

**Max errors flag tests**
- `test_max_errors_zero/`: Compile code with multiple errors using `--max-errors=0`, collect all errors in one run
- `test_max_errors_multi/`: Compile code with two parse errors, verify both are reported with `--max-errors=0`
- `test_max_errors_one/`: Compile code with multiple errors using `--max-errors=1` (or default), stop after first error
- `test_max_errors_limit/`: Test various N values (2, 3, 5) to verify error collection stops at N

### Functional Testing

1. **Version output**: Verify `ccompiler --version` displays "ClaudeC99 version MM.mm.SSSSSSSS.BBBBB"
2. **Error recovery**: With `--max-errors=0`, verify parser continues and reports errors for:
   - Missing semicolons
   - Type mismatches
   - Undeclared variables
   - Multiple errors in same file
3. **cc99 script**: Verify shell script correctly forwards both new flags to ccompiler

### Build Verification

1. CMakeLists.txt correctly builds version.c
2. Build number in version string updates after running cmake configure
3. No build errors when git commit count is extracted

### Error Collection Behavior

- With `--max-errors=2` in file with 5 errors: exactly 2 errors reported, compilation stops
- With `--max-errors=0` in file with 5 errors: all 5 errors reported
- Default behavior (no flag): stops at first error (backwards compatible)

## Known Constraints

1. Error recovery via setjmp/longjmp is synchronous; requires non-trivial changes to parser control flow
2. Version components are hardcoded; build number requires cmake configuration
3. Stage number encoding (SSMMSSSS format) must precisely match spec: main stage (2 digits) + sub stage (2 digits) + subsub (2 digits) = 00700100 for stage 70.01
