# Stage 93 Kickoff

## Summary

Stage 93 is a build system and workflow stage with no compiler language feature changes. It adds:

1. **Three build modes**: Normal (GCC/Clang), Bootstrap (pre-built ClaudeC99), Fallback (ClaudeC99 if available, else GCC/Clang)
2. **VERSION_BUILTBY tracking**: Records which compiler built the binary; shown in `--version` output as "BuiltBy: <compiler>"
3. **Timeout guards**: Every test runner and the bootstrap build script get per-invocation timeouts to prevent infinite loops

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

None required.

## Spec Issues Identified

1. Line 5: "coule be used" → "could be used" (typo)
2. Line 7: "Fallout back Build" → "Fallback Build" (typo)
3. Line 22: `VERSION_BUILTBY  Default c compiler` — not valid C; will use `#ifndef` guard with string default
4. Line 29: `"BuildBy: %s"` → `"BuiltBy: %s"` to match the macro name and consistency with spec intent

## Planned Changes

### version.c
- Update `VERSION_STAGE` to `00930000`
- Add `VERSION_BUILTBY` definition with `#ifndef` guard and fallback default string
- Update `version --version` output to include a new line showing "BuiltBy: <compiler>"
- Format: `"ClaudeC99 version %s.%s.%s.%05d\nBuiltBy: %s"`

### CMakeLists.txt
- Compute `BUILTBY_TOKEN` from cmake compiler ID and version
- Pass as `-DVERSION_BUILTBY="..."` compile definition to all targets

### build.sh (new)
- Three-mode wrapper script with argument parsing
- Normal mode: `cmake ... && make` (uses system GCC/Clang)
- Bootstrap mode: `CC=./claudec99 cmake ... && make` (uses pre-built ClaudeC99)
- Fallback mode: Try bootstrap first; if claudec99 unavailable, fall back to normal
- All builds wrapped with 300-second timeout to prevent infinite loops

### Test runners (6 files)
Each gets timeout guard wrapper around compiler and program invocations:
- `test/valid/run_tests.sh` — compiler timeout (30s), program execution timeout (30s)
- `test/invalid/run_tests.sh` — compiler timeout (30s)
- `test/integration/run_tests.sh` — compiler timeout (30s), assembler timeout (30s), program execution timeout (30s)
- `test/print_ast/run_tests.sh` — compiler timeout (30s)
- `test/print_tokens/run_tests.sh` — compiler timeout (30s)
- `test/print_asm/run_tests.sh` — compiler timeout (30s)

## Implementation Order

1. Update `src/version.c` with VERSION_STAGE and VERSION_BUILTBY
2. Update `CMakeLists.txt` to compute and pass BUILTBY_TOKEN
3. Create `build.sh` with three build modes and 300s bootstrap timeout
4. Add timeout guards to all 6 test runner scripts (30s per invocation)
5. Update README with build mode documentation

## Decisions Made

- **Timeout duration**: 30 seconds for individual test invocations (compiler, assembler, program execution), 300 seconds for complete bootstrap build
- **BUILTBY format**: CMake will extract from compiler ID and version; format will be "GCC 9.4.0", "Clang 14.0.0", "ClaudeC99 00.01.00.00093", etc.
- **Default fallback**: If VERSION_BUILTBY not defined, will use "Default C Compiler" as string constant
- **Timeout implementation**: Use `timeout` command available on Linux; will exit with code 124 on timeout

## Ambiguities Resolved

1. Timeout default: Spec did not specify duration; using 30s per test (reasonable for compilation) and 300s for bootstrap (accounts for multiple test suites running sequentially)
2. Build mode selection: Will use environment variable `CLAUDEC99_BUILD_MODE` or command-line argument to build.sh
3. Timeout tool: Using standard `timeout` command; requirement that timeouts work implies Linux/Unix environment

## Files to Create or Modify

- src/version.c (modify)
- CMakeLists.txt (modify)
- build.sh (create)
- test/valid/run_tests.sh (modify)
- test/invalid/run_tests.sh (modify)
- test/integration/run_tests.sh (modify)
- test/print_ast/run_tests.sh (modify)
- test/print_tokens/run_tests.sh (modify)
- test/print_asm/run_tests.sh (modify)
- README.md (modify - add build mode docs)
