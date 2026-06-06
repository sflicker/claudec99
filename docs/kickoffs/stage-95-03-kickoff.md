# Stage 95-03 Kickoff

## Summary

Stage 95-03 adds a reusable dynamic string/character buffer (StrBuf) module to the compiler codebase. This complements the Vec module added in stage 95-02 and provides specialized support for building strings with flexible capacity. No language features are added; no changes to tokenizer, parser, AST, or code generation are required.

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

None required.

## Spec Issues Identified

None. Spec is clear and unambiguous.

## Planned Changes

### New Files

1. **include/strbuf.h** — Public API declarations for StrBuf struct and functions
   - `StrBuf` typedef with fields: `data`, `len`, `cap`
   - Function declarations: `strbuf_init`, `strbuf_free`, `strbuf_reserve`, `strbuf_append_char`, `strbuf_append_str`, `strbuf_append_n`, `strbuf_null_terminate`

2. **src/strbuf.c** — StrBuf implementation
   - Memory management with growth policy: start at capacity 8, double on realloc
   - Overflow checks on capacity calculations
   - Fatal error handling via `fprintf(stderr, ...) + exit(1)` for allocation failures
   - `strbuf_null_terminate` writes `'\0'` at `data[len]` without incrementing `len`

3. **test/unit/test_strbuf.c** — Standalone unit test program
   - Comprehensive test coverage for all StrBuf operations
   - Simple ASSERT-based test framework
   - Test categories: init, free, append_char, append_str, append_n, null_terminate, reserve, growth, stress, mixed appends, reuse after free
   - Output format compatible with test runner: "Results: P passed, F failed, T total"

### Modified Files

1. **CMakeLists.txt** — Add `src/strbuf.c` to compiler source files list

2. **test/unit/run_tests.sh** — Update test runner to aggregate both Vec and StrBuf test results

3. **src/version.c** — Update `VERSION_STAGE` to `"00950300"`

## Implementation Order

1. Create `include/strbuf.h` with struct and API declarations
2. Create `src/strbuf.c` with full implementation
3. Update `CMakeLists.txt` to include `src/strbuf.c`
4. Create `test/unit/test_strbuf.c` with comprehensive unit tests covering all operations
5. Update `test/unit/run_tests.sh` to aggregate Vec and StrBuf test results
6. Update `src/version.c` to `"00950300"`
7. Build and run all tests to verify functionality
8. Commit implementation with appropriate message
9. Run bootstrap self-hosting test (C0→C1→C2 with full test suite)
10. Generate milestone summary artifact
11. Generate transcript artifact

## Decisions Made

- **Separate module** — New `strbuf.h`/`strbuf.c` rather than adding to Vec keeps specialized string operations distinct
- **Character-specific design** — StrBuf is optimized for char buffers (not generic like Vec), matching the common use case of building strings
- **Fatal error handling** — `strbuf.c` uses direct `fprintf(stderr, ...) + exit(1)` for allocation failures; consistent with Vec module
- **Initial capacity** — Start at 8 characters, doubling on realloc; matches growth strategy from Vec
- **Null termination** — `strbuf_null_terminate` explicitly writes sentinel without incrementing length; allows buffer to be used as C string while maintaining separate length tracking
- **Simple unit test framework** — Standalone C program with basic ASSERT macros; integrates cleanly with existing test runner via standard output format
- **Return value semantics** — `strbuf_reserve` and append functions return `int` (always 1 per spec; fatal path doesn't return)

## Ambiguities Resolved

None. Spec is clear regarding API design, growth policy, and error handling.

## Files to Create or Modify

- `include/strbuf.h` (create)
- `src/strbuf.c` (create)
- `test/unit/test_strbuf.c` (create)
- `CMakeLists.txt` (modify)
- `test/unit/run_tests.sh` (modify)
- `src/version.c` (modify)
- `docs/kickoffs/stage-95-03-kickoff.md` (create — this artifact)
- `docs/milestones/stage-95-03-milestone.md` (create — after implementation)
- `docs/sessions/stage-95-03-transcript.md` (create — after completion)
