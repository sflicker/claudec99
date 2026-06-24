# Milestone Summary

## Stage 167: Peephole Redundant Store Elimination - Complete

stage-167 ships the fifth built-in peephole optimization pattern: redundant store elimination. When two consecutive `mov [rbp - N], REG` instructions write to the same stack slot (same decimal offset, any register names), the first store is deleted and the second is kept — the first write is dead because it is overwritten before being read.

- Peephole Optimizer: Reused existing `pp_parse_store_rbp` helper from Stage 166 without modification; `match_redundant_store` compares only offsets across both lines; `replace_redundant_store` outputs the second store line and suppresses the first.
- Pattern Table: `g_builtin_patterns` expanded from 4 to 5 entries; pattern fires at `-O2`.
- Tests: One new integration test (`test_peephole_redundant_store`). All 2070 portable tests pass (165 unit, 1286 valid, 261 invalid, 187 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- System-include: 187 pass.
- Optional-library: 2 pass (test_sdl2_init, test_zlib_compress).
- Self-host: C0→C1→C2 verified with all tests passing at every stage. No source changes needed during bootstrap.
