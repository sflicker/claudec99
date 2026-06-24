# stage-167 Transcript: Peephole Redundant Store Elimination

## Summary

Stage 167 adds the fifth built-in peephole optimization pattern to the post-codegen optimizer: redundant store elimination. Code generation frequently overwrites a stack slot without reading its previous value (e.g., `mov [rbp - 48], eax` followed by `mov [rbp - 48], rbx`). After the first store, if the value is never read before being overwritten again, the first store is architecturally dead and can be eliminated. This pattern removes such dead stores by preserving the second store line and deleting the first store line when both lines target the same stack offset.

The pattern runs at `-O2` (alongside the four existing peephole patterns: zero-register idiom from Stage 157, no-op move elimination from Stage 164, push/pop pair collapse from Stage 165, and redundant reload elimination from Stage 166). Implementation reused the existing `pp_parse_store_rbp` helper from Stage 166 without modification; a new matcher verifies both store offsets match (register names are irrelevant), and a new replacer outputs only the second store line.

## Changes Made

### Step 1: Peephole Pattern Matcher and Replacer

- Added `match_redundant_store()` — accepts a 2-line window, calls `pp_parse_store_rbp` on both lines, extracts decimal offsets, and compares only the offsets for exact match (registers need not match); returns 1 if both offsets are equal, 0 otherwise.
- Added `replace_redundant_store()` — outputs only the second store line (line 1 / `win[1]`), suppressing the first store line; sets `out_count = 1`.

### Step 2: Pattern Table Expansion

- Expanded `g_builtin_patterns[]` from 4 to 5 entries.
- Registered new pattern at index 4: `window_size = 2`, `matcher = match_redundant_store`, `replacer = replace_redundant_store`.
- Updated `peephole_builtin_patterns()` to set `*n_pats = 5` instead of 4.

### Step 3: File Header and Version

- Updated `src/peephole.c` file-header comment to document Stage 167 pattern.
- Bumped `src/version.c` to `VERSION_STAGE = "01670000"`.

### Step 4: Integration Test

- Created `test/integration/test_peephole_redundant_store/` directory with three files:
  - `test_peephole_redundant_store.c` — simple function that overwrites a local variable without reading the initial value; compiled at `-O2`, the redundant first store is eliminated; returns 42 on success.
  - `test_peephole_redundant_store.cflags` — specifies `-O2` optimization flag.
  - `test_peephole_redundant_store.status` — expected exit code 42.

## Final Results

All 2070 portable tests pass:
- 165 unit
- 1286 valid
- 261 invalid
- 187 integration (was 186; +1 new test)
- 50 print-AST
- 100 print-tokens
- 21 print-asm

System-include: 187 pass
Optional-library: 2 pass (test_sdl2_init, test_zlib_compress)
Self-host C0→C1→C2 verified with all tests passing at every stage. No source changes needed during bootstrap.

## Session Flow

1. Read spec and supporting peephole infrastructure files
2. Analyzed Stage 166 redundant reload pattern to understand the matcher/replacer architecture and parse helpers
3. Designed new matcher to compare only stack offsets (register names are irrelevant)
4. Implemented pattern matcher and replacer reusing existing `pp_parse_store_rbp` helper
5. Expanded pattern table and updated initialization function
6. Added integration test demonstrating the optimization in action
7. Updated version string and file-header documentation
8. Verified all tests pass at `-O2`
9. Performed self-hosting verification (C0→C1→C2 cycle)
