# Milestone Summary

## Stage 154: Unreachable Statement Removal - Complete

Stage 154 adds unreachable statement removal to the stage-142 optimizer: in `optimize_stmt`'s `AST_BLOCK` case, after each child statement is optimized, if it is a terminal statement (return, break, continue, or goto), all subsequent siblings in the same block are freed up to the next label (exclusive) and the children array is compacted.
- Tokenizer: No changes.
- Parser: No changes.
- AST: No changes.
- Optimizer (`src/optimize.c`): Added static helper `is_terminal_stmt()` to identify the four terminal node types (`AST_RETURN_STATEMENT`, `AST_BREAK_STATEMENT`, `AST_CONTINUE_STATEMENT`, `AST_GOTO_STATEMENT`). Modified `AST_BLOCK` case in `optimize_stmt` to scan child statements after optimization, detect terminal statements, and compact the children array to remove unreachable code up to the next label. Only gated behind `-O1`. No grammar changes.
- Codegen: No changes.
- Tests: 5 new integration tests (unreachable_return, unreachable_break, unreachable_continue, unreachable_goto, unreachable_label_stop), all pass. Full suite 2042/2042 pass (165 unit, 1286 valid, 261 invalid, 159 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- Docs: `docs/outlines/checklist.md` updated with Stage 154 entry. `docs/self-compilation-report.md` updated with C0→C1→C2 self-host records.
- Notes: Labels are never removed since they are reachable via `goto`. Multiple dead zones in one block are handled by the outer loop continuing past each label. Only direct block-level children are checked—nested blocks ending in a terminal do not trigger removal of outer siblings. Self-host C0→C1→C2 verified with all tests passing at every stage.
