# Milestone Summary

## Stage 156: -O1 Switch Dead-Code Removal Fix - Complete

Stage 156 ships a critical correctness bug fix for the `-O1` optimizer's dead-code elimination in switch statements (CC99-014).

- **Bug**: The AST optimizer's block-level dead-code scan (after terminal statements like `break`) did not recognize `AST_CASE_SECTION` and `AST_DEFAULT_SECTION` as control-flow boundaries. When a `break` statement appeared inside a switch case, all subsequent case sections in the block were incorrectly removed as "dead code," silently breaking switch state-machine loops.
- **Root cause**: In `src/optimize.c`, the `AST_BLOCK` dead-code removal loop only stopped at `AST_LABEL_STATEMENT`, not at case/default sections.
- **Fix**: One-line change to the while condition in `optimize_stmt`'s `AST_BLOCK` case — added `AST_CASE_SECTION` and `AST_DEFAULT_SECTION` as stop conditions alongside `AST_LABEL_STATEMENT`.
- **Tokenizer/Parser/AST**: No changes; bug was purely in the optimizer.
- **Tests**: 4 new integration tests added (test_switch_break_o0, test_switch_break_o1, test_switch_break_default_o1, test_switch_state_update_o1); all exercise switch loops with -O1 optimization.
- **Verification**: All 2049 portable tests pass (165 unit, 1286 valid, 261 invalid, 166 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage; no source changes needed during bootstrap.
- **Notes**: The bug manifested as a silent correctness regression in enum-based state machines compiled with `-O1`. With the fix, behavior now matches `-O0` and GCC for all cases.
