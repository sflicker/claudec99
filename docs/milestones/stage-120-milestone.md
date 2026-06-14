# Milestone Summary

## Stage 120: FP Increment/Decrement on Struct Members - Complete

stage-120 adds support for prefix and postfix `++`/`--` operators on `float` and `double` struct fields accessed via dot (`.`) and arrow (`->`) operators.

- **Tokenizer**: No changes.
- **Grammar/Parser**: No changes.
- **AST/Semantics**: No changes.
- **Codegen**: Fixed two bugs in `codegen_inc_dec_general` by adding an FP early-return path that uses SSE2 instructions (`movsd`/`movss`, `addsd`/`addss`, `subsd`/`subss`). Bug 1: `TYPE_DOUBLE` fell to `default: sz = 4` in the fallback size switch (should be 8 bytes). Bug 2: integer `add`/`sub` instructions were used regardless of type. Added two new `.rodata` constants (`Lfp_one_f64` and `Lfp_one_f32`) for the 1.0 increment/decrement operands. New flags `fp_one_f64_emitted` and `fp_one_f32_emitted` track whether these constants have been referenced.
- **Tests**: 7 new tests added to `test/valid/structs/`: prefix increment/decrement on double fields, postfix increment/decrement returning old values, arrow member access, float field support, and loop accumulation. Full suite: 1886 passed, 0 failed.
- **Docs**: `src/version.c` bumped to `"01200000"`.
- **Notes**: Bonus fixes included: the same FP path also corrects `++`/`--` on FP array elements and FP pointer dereferences, since they all route through `codegen_inc_dec_general`. Self-host C0→C1→C2 all passed 1886 tests with no source changes during bootstrap.
