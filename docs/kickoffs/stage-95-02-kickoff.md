# Stage 95-02 Kickoff

## Summary

Stage 95-02 adds a reusable generic growable-array (Vec) module to the compiler codebase. This is a foundational utility for future stages that will replace fixed-capacity tables throughout the compiler. No language features are added; no changes to tokenizer, parser, AST, or code generation are required.

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

None required.

## Spec Issues Identified

One minor typo in the spec: "memory usage directory" likely means "memory usage directly" in the context of testing the new Vec module. Intent is clear and does not affect implementation.

## Planned Changes

### New Files

1. **include/vec.h** — Public API declarations for Vec struct and functions
   - `Vec` typedef with fields: `data`, `len`, `cap`, `elem_size`
   - Function declarations: `vec_init`, `vec_free`, `vec_reserve`, `vec_push`, `vec_get`, `vec_pop`, `vec_clear`

2. **src/vec.c** — Vec implementation
   - Memory management with growth policy: start at 0, double on push (min 8 on first alloc)
   - Overflow checks on capacity calculations
   - Fatal error handling via `fprintf(stderr, ...) + exit(1)` for allocation failures

3. **test/unit/test_vec.c** — Standalone unit test program
   - Comprehensive test coverage for all Vec operations
   - Simple ASSERT-based test framework
   - Output format compatible with test runner: "Results: P passed, F failed, T total"

4. **test/unit/run_tests.sh** — Test runner script for unit tests
   - Compiles and executes test_vec.c
   - Captures test results and exit code

### Modified Files

1. **CMakeLists.txt** — Add `src/vec.c` to compiler source files list

2. **test/run_all_tests.sh** — Add `unit` test suite to the test runner

3. **src/version.c** — Update `VERSION_STAGE` to `"00950200"`

## Implementation Order

1. Create `include/vec.h` with struct and API declarations
2. Create `src/vec.c` with full implementation
3. Update `CMakeLists.txt` to include `src/vec.c`
4. Create `test/unit/test_vec.c` with comprehensive unit tests
5. Create `test/unit/run_tests.sh` test runner script
6. Update `test/run_all_tests.sh` to include unit test suite
7. Update `src/version.c` to `"00950200"`
8. Build and run all tests to verify functionality
9. Commit implementation with appropriate message
10. Run bootstrap self-hosting test (C0→C1→C2 with full test suite)
11. Generate milestone summary artifact
12. Generate transcript artifact

## Decisions Made

- **Separate module** — New `vec.h`/`vec.c` rather than adding to existing `util` module keeps concerns separate and maintains reusability
- **Fatal error handling** — `vec.c` uses direct `fprintf(stderr, ...) + exit(1)` for allocation failures; no dependency on `compile_error` from util.h
- **Lazy initialization** — Initial capacity is 0; allocation happens on first push
- **Growth strategy** — Double capacity on push when full; minimum of 8 on first allocation
- **Simple unit test framework** — Standalone C program with basic ASSERT macros; integrates cleanly with existing test runner via standard output format
- **Return value semantics** — `vec_reserve` and `vec_push` return `int` (always 1 per spec; fatal path doesn't return)

## Ambiguities Resolved

None. Spec is clear regarding API design, growth policy, and error handling.

## Files to Create or Modify

- `include/vec.h` (create)
- `src/vec.c` (create)
- `test/unit/test_vec.c` (create)
- `test/unit/run_tests.sh` (create)
- `CMakeLists.txt` (modify)
- `test/run_all_tests.sh` (modify)
- `src/version.c` (modify)
- `docs/kickoffs/stage-95-02-kickoff.md` (create — this artifact)
- `docs/milestones/stage-95-02-milestone.md` (create — after implementation)
- `docs/sessions/stage-95-02-transcript.md` (create — after completion)
