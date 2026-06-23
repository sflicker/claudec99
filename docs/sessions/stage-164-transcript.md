# stage-164 Transcript: Peephole No-op Move Elimination

## Summary

Stage 164 adds the second built-in peephole optimization pattern to `src/peephole.c`: no-op move elimination. When a `mov REG, REG` instruction has the same register on both the source and destination sides (e.g. `mov rax, rax`), the instruction performs no work and is deleted entirely from the output.

The pattern fires at `-O2` via the existing sliding-window engine established in Stage 155. Unlike the Stage 157 zero-register pattern (which required parallel lookup tables for register name mapping), this pattern only needs a string comparison: parse the destination register name from the `mov` operand, then check whether the source operand string is identical. This approach covers all register widths (64-bit `rax`, 32-bit `eax`, 16-bit `ax`, 8-bit `al`, and extended registers `r8`–`r15` in all widths) without any table.

No tokenizer, parser, AST, or codegen changes were needed. The entire change is confined to `src/peephole.c`.

## Changes Made

### Step 1: Peephole pattern (`src/peephole.c`)

- Added `match_nop_move(const char **win, int n)` — skips leading whitespace, checks for `"mov "` prefix, extracts the destination register name up to the `,` separator, verifies the source token (after `", "`) equals the destination token and the line ends there; returns 1 on match, 0 otherwise
- Added `replace_nop_move(const char **win, int n, char **out, int *out_count)` — sets `*out_count = 0` to delete the matched line entirely (no output lines)
- Expanded `g_builtin_patterns[1]` → `g_builtin_patterns[2]` to accommodate the second entry
- Registered the new pattern at index `[1]`: `window_size = 1`, `matcher = match_nop_move`, `replacer = replace_nop_move`
- Updated `*n_pats` return value from `1` to `2` in `peephole_builtin_patterns()`
- Updated file-header comment to mention Stage 164

### Step 2: Integration test (`test/integration/test_peephole_nop_move/`)

- `test_peephole_nop_move.c`: simple `identity(int x)` function plus `main` returning 42 via the identity call — exercises the `-O2` path end-to-end
- `test_peephole_nop_move.cflags`: `-O2`
- `test_peephole_nop_move.status`: `42`

### Step 3: Version bump (`src/version.c`)

- `VERSION_STAGE` updated from `"01630000"` to `"01640000"`

## Final Results

Build: clean (GCC, `cmake --build build`).

All 2067 portable tests pass (165 unit, 1286 valid, 261 invalid, 184 integration [+1 new], 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 184 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress).

Self-host: C0→C1→C2 verified with no source changes during bootstrap.

```
C0: ClaudeC99 version 00.03.01640000.01213
C1: ClaudeC99 version 00.03.01640000.01214
C2: ClaudeC99 version 00.03.01640000.01215
```

No regressions.

## Session Flow

1. Read spec `docs/stages/ClaudeC99-spec-stage-164-peephole-nop-move-elimination.md` and supporting files
2. Reviewed `src/peephole.c` and `include/peephole.h` to understand existing pattern infrastructure
3. Generated kickoff artifact via haiku-stage-artifact-writer
4. Implemented `match_nop_move` and `replace_nop_move` in `src/peephole.c`; expanded pattern table
5. Created integration test `test/integration/test_peephole_nop_move/`
6. Built with GCC; all 2067 portable tests pass
7. Bumped `VERSION_STAGE` to `"01640000"` in `src/version.c`
8. Committed implementation
9. Ran `./build.sh --mode=self-host` — C0→C1→C2 verified, all tests pass at every stage
10. Updated `docs/self-compilation-report.md` and committed
11. Generated milestone, transcript, checklist, and README artifacts

## Commit

`9ef0037` — stage 164: peephole no-op move elimination
