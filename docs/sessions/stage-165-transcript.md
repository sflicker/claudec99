# stage-165 Transcript: Peephole Push/Pop Pair Collapse

## Summary

Stage 165 adds the third built-in peephole optimization pattern to `src/peephole.c`: push/pop pair collapse. When a `push rX` instruction is immediately followed by a `pop rY` instruction (adjacent lines, no intervening branch or label), the pair is collapsed into a single `mov rY, rX`, eliminating a stack memory round-trip and reducing text-segment size. This is the first 2-line window pattern in the peephole optimizer (patterns 0 and 1 used single-line windows).

The pattern fires at `-O2` via the existing sliding-window engine established in Stage 155. The special case where both registers are identical (`push rX` / `pop rX`) is handled by deleting both lines as a no-op, since pushing a register and immediately popping it to the same register has no effect.

No tokenizer, parser, AST, or codegen changes were needed. The entire change is confined to `src/peephole.c`, with all new code written in C89 style for bootstrap compatibility (declarations at top of block, no VLAs, no // comments).

## Changes Made

### Step 1: Peephole pattern helpers (`src/peephole.c`)

- Added `pp_extract_reg(const char *line, const char *mnemonic, char *dst, size_t dst_size)` — a helper that strips leading whitespace, checks for the specified mnemonic prefix (`"push "` or `"pop "`), extracts the register name up to the line end, and copies it to the destination buffer; returns 1 on success, 0 on failure (missing mnemonic, missing register, or destination buffer too small)
- Helper implementation follows C89 strict declarative style (all variables at block start)

### Step 2: Pattern matcher (`src/peephole.c`)

- Added `match_push_pop(const char **win, int n)` — uses `pp_extract_reg` to extract register names from both `push` (window line 0) and `pop` (window line 1) instructions; returns 1 if both extraction calls succeed, 0 otherwise

### Step 3: Pattern replacer (`src/peephole.c`)

- Added `replace_push_pop(const char **win, int n, char **out, int *out_count)` — uses `pp_extract_reg` to extract both register names, preserves the leading whitespace (spaces and tabs) from the `push` line, handles two cases: (a) if registers match, sets `*out_count = 0` to delete both lines; (b) if registers differ, constructs a `mov rY, rX` instruction with the preserved leading whitespace and stores it in `out[0]` with `*out_count = 1`

### Step 4: Pattern registration (`src/peephole.c`)

- Expanded `g_builtin_patterns` static array from `[2]` to `[3]`
- Registered pattern at index `[2]`: `window_size = 2` (first 2-line window pattern), `matcher = match_push_pop`, `replacer = replace_push_pop`
- Updated `*n_pats` return value from `2` to `3` in `peephole_builtin_patterns()`
- Updated file-header comment to mention Stage 165

### Step 5: Integration test (`test/integration/test_peephole_push_pop/`)

- `test_peephole_push_pop.c`: defines `add(int, int)` function and calls it twice in `main` — exercises the push/pop pair collapse at `-O2` and verifies semantic correctness
- `test_peephole_push_pop.cflags`: `-O2`
- `test_peephole_push_pop.status`: `42`

### Step 6: Version bump (`src/version.c`)

- `VERSION_STAGE` updated from `"01640000"` to `"01650000"`

## Final Results

Build: clean (GCC, `cmake --build build`).

All 2068 portable tests pass (165 unit, 1286 valid, 261 invalid, 185 integration [+1 new], 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 185 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress).

Self-host: C0→C1→C2 verified with no source changes during bootstrap.

```
C0: ClaudeC99 version 00.03.01650000.01219
C1: ClaudeC99 version 00.03.01650000.01220
C2: ClaudeC99 version 00.03.01650000.01221
```

Pattern verification: Comparing `-O0` and `-O2` assembly output for the test file, adjacent `push rax / pop rsi` pairs (and other register combinations) visible in `-O0` output are collapsed to `mov rsi, rax` in `-O2` output.

No regressions.

## Session Flow

1. Read spec `docs/stages/ClaudeC99-spec-stage-165-peephole-push-pop-collapse.md` and supporting files
2. Reviewed `src/peephole.c`, `include/peephole.h`, and existing patterns (zero-reg, nop-move) to understand infrastructure
3. Reviewed Stage 164 milestone and transcript as recent examples
4. Implemented `pp_extract_reg`, `match_push_pop`, and `replace_push_pop` in `src/peephole.c` in C89 style
5. Expanded `g_builtin_patterns[2]` → `[3]` and registered the new 2-line window pattern; updated `*n_pats` to 3
6. Created integration test `test/integration/test_peephole_push_pop/` with function-call test case
7. Built with GCC; all 2068 portable tests pass
8. Bumped `VERSION_STAGE` to `"01650000"` in `src/version.c`
9. Ran `./build.sh --mode=self-host` — C0→C1→C2 verified, all tests pass at every stage
10. Generated milestone, transcript, and README-update artifacts

## Commit

`844d1c7` — stage 165 session usage and export (in ongoing documentation batch)
