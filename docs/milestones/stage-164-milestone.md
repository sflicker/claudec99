# Milestone Summary

## Stage 164: Peephole No-op Move Elimination - Complete

stage-164 ships the second built-in peephole optimization pattern: no-op move elimination. When a `mov REG, REG` instruction has the same register on both source and destination sides (e.g. `mov rax, rax`), the instruction is deleted entirely.

- Peephole: Added `match_nop_move` matcher and `replace_nop_move` replacer in `src/peephole.c`. Pattern fires at `-O2` for all register widths (64-bit, 32-bit, 16-bit, 8-bit, and extended registers r8–r15).
- Tests: 1 new integration test (`test_peephole_nop_move`: identity function compiled with `-O2`, returns 42). Full suite: 2067 portable tests pass (165 unit, 1286 valid, 261 invalid, 184 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 184 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress).
- Docs: README updated with Stage 164 entry. Checklist updated. Milestone and transcript added.
- Version: `src/version.c` incremented to `01640000`.
- Notes: Self-host C0→C1→C2 verified with all tests passing at every stage. Pattern uses generic string-comparison matcher; no register table needed.
