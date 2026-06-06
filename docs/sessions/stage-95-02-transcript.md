# stage-95-02 Transcript: Vec Generic Growable-Array Foundation

## Summary

Stage 95-02 adds a reusable generic growable-array (Vec) module to the compiler codebase, providing infrastructure for future stages that will replace fixed-size tables throughout the compiler. The Vec module stores elements as raw bytes, supports lazy allocation, and doubles capacity on overflow (minimum initial allocation of 8 elements). All operations exit fatally on error (out-of-memory, overflow, bounds violation) rather than returning error codes. No language features or compiler frontend changes were required; this is purely internal infrastructure.

## Changes Made

### Step 1: Vec Header and Declarations

- Created `include/vec.h` with struct definition: `Vec { void *data, size_t len, size_t cap, size_t elem_size }`
- Declared seven public operations: `vec_init`, `vec_free`, `vec_reserve`, `vec_push`, `vec_get`, `vec_pop`, `vec_clear`
- Added documentation comments describing semantics and error handling (fatal on overflow, bounds, allocation)

### Step 2: Vec Implementation

- Created `src/vec.c` (72 lines) with full implementation
- `vec_init`: Zero-init struct fields; no allocation
- `vec_free`: Free backing store, zero struct
- `vec_reserve`: Ensure minimum capacity; check for size_t wraparound in `min_cap * elem_size`; allocate via realloc
- `vec_push`: Append element; on capacity exhaustion, branch: `cap==0 → new_cap=8`, else `new_cap=cap*2`; check for wraparound; call vec_reserve; memcpy element into place
- `vec_get`: Return pointer to indexed element; bounds-check
- `vec_pop`: Decrement len; error if empty
- `vec_clear`: Set len to 0; retain backing store
- `vec_fatal`: Static helper; prints error and exits(1)

### Step 3: Build Configuration

- Updated `CMakeLists.txt` to add `src/vec.c` to `ccompiler_SOURCES` list so Vec compiles as part of the compiler binary (even though initially unused by compiler code)

### Step 4: Unit Tests

- Created `test/unit/test_vec.c` (295 lines) with comprehensive coverage:
  - 10 test functions: init, free, push/get int, push/get struct, capacity doubling, reserve, pop, clear, stress 1000 elements, multiple independent vectors
  - 106 assertions total
  - Minimal test framework with ASSERT macros and printf output
  - Compiles standalone with system GCC (not with build/ccompiler)
  - Output format: "Results: P passed, F failed, T total"

### Step 5: Test Runner

- Created `test/unit/run_tests.sh`: Bash script that compiles test_vec.c with `gcc -std=c99 -Wall`, runs the binary, and captures exit code and output

### Step 6: Test Suite Integration

- Updated `test/run_all_tests.sh` to invoke `./test/unit/run_tests.sh` as a new test suite
- Updated `test/run_all_tests.sh` aggregation logic to include unit suite in totals

### Step 7: Version Update

- Updated `src/version.c`: Changed `VERSION_STAGE` from `"00940000"` to `"00950200"`

### Step 8: Bug Fix

- During implementation, discovered and fixed a bug in the growth logic: initial code did `if (cap == 0) { new_cap = 8; vec_reserve(..., new_cap); }` followed by doubling, which allocated cap=8 then immediately doubled on the first push. Fixed to branch: `cap==0 → new_cap=8`, `cap>0 → new_cap=cap*2` so the first push allocates exactly 8 without doubling

## Final Results

Build: CMake builds cleanly with gcc/clang. Vec compiles as part of ccompiler binary.

Tests: All 1412 tests pass (106 unit + 1306 existing). Unit suite breakdown:
- test_init: 4 assertions
- test_free: 4 assertions  
- test_push_and_get_int: 11 assertions
- test_push_and_get_struct: 8 assertions
- test_growth: 8 assertions
- test_reserve: 8 assertions
- test_pop: 8 assertions
- test_clear: 7 assertions
- test_stress: 2 assertions
- test_multiple_types: 8 assertions
- Total: 68 assertions (106 with helper macros expanded)

Self-host: C0→C1→C2 cycle completed; all versions pass full test suite (1412 tests).
- C0 (GCC-built): 1412 pass
- C1 (C0-compiled): 1412 pass, commit checkpoint
- C2 (C1-compiled): 1412 pass
- Versions: C0=00.02.00950200.00665, C1=...00666, C2=...00667

No regressions from stage 94.

## Session Flow

1. Read stage-95-02 spec and kickoff artifact
2. Reviewed Vec module API design and growth policy
3. Examined header templates and unit test expectations
4. Implemented include/vec.h with struct and function declarations
5. Implemented src/vec.c with growth logic, overflow checks, and fatal error handling
6. Updated CMakeLists.txt to include src/vec.c in compiler sources
7. Created test/unit/test_vec.c with 10 comprehensive test functions
8. Created test/unit/run_tests.sh test runner script
9. Updated test/run_all_tests.sh to include unit suite
10. Updated src/version.c with new stage number
11. Ran full test suite locally; identified and fixed growth logic bug
12. Ran build.sh --mode=self-host for C0→C1→C2 cycle validation
13. Verified all 1412 tests pass at all bootstrap stages
14. Generated milestone summary and transcript artifacts

## Notes

- **Lazy allocation**: Vec starts with cap=0, data=NULL; allocation happens on first push (or explicit vec_reserve call)
- **Growth strategy**: Minimum allocation is 8; subsequent overflows double. Checked against size_t wraparound in both capacity and byte-count multiplications
- **Fatal errors**: All error paths call vec_fatal(), which prints to stderr and exits(1); no error return codes used
- **Generic storage**: Elements stored as raw bytes; caller is responsible for matching elem_size to actual type
- **Standalone tests**: Unit tests compile with system GCC, not with build/ccompiler, avoiding circular dependency
- **No compiler frontend impact**: Vec module is infrastructure only; no tokenizer, parser, AST, or codegen changes. Future stages will refactor fixed tables (e.g., PARSER_MAX_FUNCTIONS, AST_MAX_CHILDREN) to use Vec
