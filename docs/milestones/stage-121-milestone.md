# Milestone Summary

## Stage 121: Switch on Long/Long Long Discriminants - Complete

stage-121 fixes the `switch` statement so that `long`, `long long`, and `unsigned long long` discriminants are compared with 64-bit instructions instead of 32-bit instructions, which were truncating the high bits of 64-bit values and causing incorrect case matching.

- **Tokenizer**: No changes.
- **Grammar/Parser**: No changes.
- **AST/Semantics**: No changes.
- **Codegen**: Fixed the switch dispatch loop in `codegen_statement` to emit 64-bit load and compare instructions when the discriminant type is `long`, `long long`, or `unsigned long long`. Previously, `mov eax, [rsp]` (32-bit) was emitted unconditionally, truncating the high 32 bits. Now the code captures the discriminant's `result_type` immediately after `codegen_expression` and emits `mov rax, [rsp]` / `cmp rax, <val>` for 64-bit types, keeping the 32-bit form for `int`/`char`/`short` which are promoted to `int` before evaluation.
- **Tests**: 6 new tests added to `test/valid/statements/`: switch on `long` with small values, switch on `long` with default case, switch on `long long`, switch on `unsigned long long`, regression tests for `char` and `int` to ensure no regression. Full suite: 1892 passed, 0 failed (up from 1886).
- **Docs**: `src/version.c` bumped to `"01210000"`.
- **Notes**: Case values outside `[−2³¹, 2³¹−1]` for 64-bit discriminants remain out of scope (would require `mov rcx, imm64; cmp rax, rcx` encoding). The compiler's own source uses `switch` only on `int`-typed enums, so C0→C1→C2 bootstrap produced identical code with no source changes needed.
