# Stage 157 Transcript: Zero-Register Peephole Pattern

## Summary

Stage 157 ships the first peephole optimization pattern, replacing the inefficient `mov REG, 0` instruction with `xor REG32, REG32` — a 2-byte encoding versus 5–7 bytes for mov-immediate, reducing code size and improving performance. The pattern is registered as a built-in matcher-replacer pair and activated at the `-O2` optimization level (the peephole pass runs alongside the AST optimizer at `-O2`, while `-O0` and `-O1` skip both). The implementation adds helper functions to parse and match the pattern across all 20 general-purpose register pairs (14 64-bit: rax–r15 minus rsp/rbp; 6 32-bit: eax–r8d plus variants), register lookup tables for efficient mapping, and a lazy runtime initializer to work around a C0 bootstrap limitation (C0 does not support function-pointer aggregate struct initialization).

## Changes Made

### Step 1: Peephole Pattern Tables and Helpers

- Added `g_zr_src[20]` and `g_zr_dst[20]` static register lookup tables in `src/peephole.c` mapping source registers to their 32-bit counterparts for zero-initialization (e.g., `rax` → `eax`, `r9` → `r9d`)
- Implemented `zr_find_reg(const char *line, const char **reg_out)` helper to parse the `mov` instruction and extract the register operand from lines like `mov rax, 0` or `mov eax, 0`
- Implemented `match_zero_reg(const char * const *window, int window_size, int *matched_register)` matcher function that calls `zr_find_reg`, searches for the matched register in the lookup table, and returns 1 if found
- Implemented `replace_zero_reg(const char * const *window, int window_size, int matched_register, Vec *output)` replacer function that emits `xor REG32d, REG32d` for the matched register pair and returns the number of lines consumed

### Step 2: Pattern Registration and Lazy Initialization

- Created `g_builtin_patterns[1]` static array holding a single `PeepholePattern` struct with `match_zero_reg` and `replace_zero_reg` function pointers
- Added `peephole_builtin_patterns()` accessor function in `src/peephole.c` with lazy runtime initialization: on first call, populates `g_zr_src` and `g_zr_dst` tables, then returns a pointer to `g_builtin_patterns` array
- Added function declaration to `include/peephole.h`: `const PeepholePattern *peephole_builtin_patterns(int *out_count)`
- Lazy initialization strategy avoids the C0 bootstrap issue where aggregate struct initialization with function pointers is not supported; initialization happens at runtime instead

### Step 3: Compiler Integration

- Updated `src/compiler.c` to call `peephole_builtin_patterns(&pattern_count)` and pass the result to `peephole_run_file()` instead of `NULL` at `-O2` optimization level
- Modified the call site in the pipeline to wire in the pattern table only when `-O2` is active

### Step 4: Integration Tests

- Created `test/integration/test_zero_reg_int/` — function initializing local `int x = 0` with `-O2`, expecting `xor eax, eax` or equivalent in the assembly
- Created `test/integration/test_zero_reg_long/` — function initializing local `long x = 0` with `-O2`, expecting `xor rax, rax` in the assembly
- Created `test/integration/test_zero_reg_arithmetic/` — arithmetic expression with zero initialization at `-O2` (correctness check)
- Created `test/integration/test_zero_reg_logical/` — logical operation with zero initialization at `-O2` (correctness check)
- Each test includes `.cflags` specifying `-O2` to activate the peephole pass

### Step 5: Bootstrap Fixes

Two issues were discovered and fixed during C0→C1→C2 self-hosting:

1. **C0 Function-Pointer Aggregate Initialization**: C0 does not support initializing structs with aggregate literals that contain function-pointer fields. The initial implementation attempted to initialize `g_builtin_patterns` at compile time; this was replaced with lazy runtime initialization via `peephole_builtin_patterns()`, which calls `vec_init` and populates the register tables on first invocation — transparent to callers and C0-compatible.

2. **C0 Pointer Arithmetic with Triple Indirection**: C0 generates incorrect code for pointer arithmetic like `*ptr + i` when `ptr` is of type `char ***`. In `src/peephole.c`, the line-reading loop initially used `for (int i = 0; i < file_line_count; i++) { char ***ptr = &lines; const char *line = *ptr[i]; }` which C0 miscompiled. Fixed by introducing a local intermediate variable: `char **arr = *lines; const char *line = arr[i];` so the pointer arithmetic is limited to a single dereference level.

### Step 6: Verification

- Built and ran full test suite: all 2053 portable tests pass
  - 165 unit assertions
  - 1286 valid C programs
  - 261 invalid C programs (error detection)
  - 170 integration tests (4 new for stage 157)
  - 50 print-AST tests
  - 100 print-tokens tests
  - 21 print-asm tests
- Self-hosting verification: compiler successfully compiled itself through the C0→C1→C2 bootstrap cycle
- All 2053 tests pass at C0, C1, and C2 stages — no regressions, no intermediate source changes needed

## Final Results

All 2053 portable tests pass. Self-host C0→C1→C2 verified: all tests pass at every stage. No regressions.

## Session Flow

1. Read the stage 157 spec describing the zero-register peephole optimization pattern
2. Reviewed the peephole infrastructure from stage 155 and the pattern registration mechanism
3. Designed the register lookup tables and parsing logic for `mov REG, 0` instructions
4. Implemented `match_zero_reg` and `replace_zero_reg` functions in `src/peephole.c`
5. Wired in the `peephole_builtin_patterns()` accessor with lazy initialization for C0 compatibility
6. Updated the compiler pipeline to call the accessor and pass patterns to the peephole pass at `-O2`
7. Created four integration tests covering various zero-initialization scenarios
8. Attempted initial build and discovered the C0 function-pointer aggregate initialization issue
9. Refactored to use lazy runtime initialization instead of compile-time struct initialization
10. Discovered and fixed the C0 triple-indirection pointer arithmetic bug in the line-reading loop
11. Built the compiler and ran the full test suite — all tests passing
12. Verified self-hosting via the C0→C1→C2 bootstrap cycle
13. Recorded final results and completion notes
