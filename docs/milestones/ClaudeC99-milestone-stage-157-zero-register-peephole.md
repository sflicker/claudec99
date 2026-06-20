# Milestone Summary

## Stage 157: Zero-Register Peephole Pattern - Complete

Stage 157 ships the first peephole optimization pattern: the zero-register idiom, replacing `mov REG, 0` with `xor REG32, REG32` (a 2-byte encoding vs. 5–7 bytes for mov-immediate).

- **Peephole Pattern:** Added zero-register matcher and replacer in `src/peephole.c`; covers all 14 64-bit and 6 32-bit general-purpose registers (20 register pairs). Single-line window pattern triggered at `-O2` optimization level.
- **Pattern Registration:** Implemented `g_builtin_patterns[]` static array and `peephole_builtin_patterns()` accessor in `src/peephole.c`; added function declaration to `include/peephole.h`. Used lazy runtime initialization (C0 does not support function-pointer aggregate initializers).
- **Compiler Integration:** Updated `src/compiler.c` to call `peephole_builtin_patterns()` and pass the result to `peephole_run_file()` instead of NULL/0.
- **Helper Functions:** Added `zr_find_reg()` to parse and match `mov REG, 0` lines; added register lookup tables `g_zr_src[]` and `g_zr_dst[]` for parallel register mapping.
- **Tests:** 4 new integration tests (`test_zero_reg_int`, `test_zero_reg_long`, `test_zero_reg_arithmetic`, `test_zero_reg_logical`) verifying correctness of zero-register replacement at `-O2`.
- **Bootstrap Fixes:** Two issues required source changes: C0 does not support function-pointer initializers in struct aggregate literals (fixed with lazy runtime init); C0 generates incorrect pointer arithmetic for `*ptr + i` (fixed with local intermediate variable). All 2053 tests pass (165 unit, 1286 valid, 261 invalid, 170 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- **Docs:** Updated `src/version.c` (VERSION_STAGE "01570000"), `docs/outlines/checklist.md` (marked Stage 157 complete, added Stage 157 section).
