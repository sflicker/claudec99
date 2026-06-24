# Milestone Summary

## Stage 168: Peephole Dead-Jump Removal - Complete

stage-168 ships the sixth built-in peephole optimization pattern: dead-jump removal. When `jmp .Lxx` is immediately followed by the label `.Lxx:`, the jump is a no-op (execution falls through anyway) and is deleted.

- Peephole Optimizer: Two new parse helpers (`pp_parse_jmp_label`, `pp_parse_label_def`) extract the jump target and label name; `match_dead_jump` compares them for equality; `replace_dead_jump` outputs only the label line and suppresses the jump.
- Pattern Table: `g_builtin_patterns` expanded from 5 to 6 entries; pattern fires at `-O2`.
- Tests: One new integration test (`test_peephole_dead_jump`) using a `goto`-to-adjacent-label pattern (natural trigger in codegen). All 2071 portable tests pass (165 unit, 1286 valid, 261 invalid, 188 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- System-include: 188 pass.
- Optional-library: 2 pass (test_sdl2_init, test_zlib_compress).
- Self-host: C0→C1→C2 verified with all tests passing at every stage. No source changes needed during bootstrap.
