# stage-92 Kickoff: Self-Compilation Validation

## Summary

Validate that the compiler can successfully compile itself in a multi-stage bootstrap: build compiler C0 (using system gcc/clang), use C0 to compile its own `src/` into C1, run the full test suite with C1 (all 1302 tests must pass), bump VERSION_MINOR, use C1 to compile into C2, run the full test suite with C2 (all must pass), and update README.md to record the self-compilation achievement.

## Required Tokenizer Changes

No changes — validation-only stage.

## Required Parser Changes

No changes — validation-only stage.

## Required AST Changes

No changes — validation-only stage.

## Required Code Generation Changes

No changes — validation-only stage.

## Version and Build Changes

1. **src/version.c**:
   - Update `VERSION_STAGE` from `"00910000"` to `"00920000"`
   - Update `VERSION_MINOR` from `"01"` to `"02"` (only when building C2 from C1)

2. **test/self_compile_validate.sh** (new script):
   - Bootstrap C0 using CMake + system compiler
   - Use C0 to compile `src/` into C1 executable
   - Run full test suite with C1
   - Update `VERSION_MINOR` to "02" in version.c
   - Use C1 to compile `src/` into C2 executable
   - Run full test suite with C2
   - Report success/failure and version strings

3. **README.md**:
   - Add a section documenting self-compilation achievement (version C2)

## Test Plan

1. Build C0 using CMake with system compiler (gcc/clang)
2. Run C0 to compile `src/` into C1 binary
3. Run full test suite (`test/run_all_tests.sh`) with C1:
   - Six test suites: tokens, parse, codegen, compile, link, runtime
   - Expected: 1302 total tests, all passing
4. Update version in `src/version.c` (VERSION_MINOR "01" → "02")
5. Run C1 to compile `src/` into C2 binary
6. Run full test suite with C2:
   - Same six suites, 1302 total tests
   - Expected: all passing
7. Verify C2's version string reports stage 92, minor 02
8. Update README.md with self-compilation achievement

## Implementation Order

1. Create test/self_compile_validate.sh script
2. Update src/version.c VERSION_STAGE to "00920000"
3. Execute the bootstrap validation sequence
4. Update README.md to document achievement
5. Verify all tests pass at both bootstrap generations
