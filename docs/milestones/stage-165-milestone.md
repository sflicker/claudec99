# Milestone Summary

## Stage 165: Peephole Push/Pop Pair Collapse - Complete

stage-165 ships the third built-in peephole optimization pattern: push/pop pair collapse. Adjacent `push rX` / `pop rY` instructions are collapsed into a single `mov rY, rX`, eliminating a stack round-trip.

- Peephole: Added `pp_extract_reg` helper, `match_push_pop` matcher, and `replace_push_pop` replacer in `src/peephole.c`. This is the first 2-line window pattern (patterns 0 and 1 used single-line windows). Same-register pairs (`push rX` / `pop rX`) are deleted entirely; cross-register pairs become `mov rY, rX` with leading whitespace preserved.
- Tests: 1 new integration test (`test_peephole_push_pop`: calls function twice, compiled with `-O2`, returns 42). Full suite: 2068 portable tests pass (165 unit, 1286 valid, 261 invalid, 185 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 185 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress).
- Docs: README updated with Stage 165 entry. Checklist updated. Milestone and transcript added.
- Version: `src/version.c` incremented to `01650000`.
- Bootstrap: C0→C1→C2 verified with all tests passing at every stage. New C89-style implementation (all declarations at top of block, no VLAs, no // comments) compiled cleanly under C0.
