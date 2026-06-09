# Milestone Summary

## stage-95-12: Fix #if Unary Overflow and Dynamic Switch Labels - Complete

stage-95-12 ships two remaining fixed-capacity conversions: the preprocessor's unbounded `#if`/`#elif` unary-operator chain (now using StrBuf) and the code generator's per-switch label cap (now using Vec).

- **Preprocessor**: `eval_cond_unary` in src/preprocessor.c replaced the fixed `char ops[32]` buffer with a dynamic `StrBuf`, eliminating overflow when handling unary-operator chains longer than 32 (e.g., `!-~+1`). Operators are appended as consumed and applied right-to-left.
- **Code Generator**: `SwitchCtx` struct in include/codegen.h converted from parallel fixed arrays (`nodes[MAX_SWITCH_LABELS]` and `labels[MAX_SWITCH_LABELS]` + count) to a single embedded `Vec entries; /* SwitchLabel */` structure. The per-switch case/default capacity (256) is eliminated entirely; `MAX_SWITCH_LABELS` removed from include/constants.h. One bug fixed during testing: dangling-pointer dereference in post-body cleanup when nested switches reallocate the stack; fixed by re-fetching the live top element before freeing.
- **Tests**: Added `test_pp_if_long_unary_chain__42.c` (100 unary operators applied right-to-left) and `test_switch_over_256_labels__77.c` (300-case switch). All 1481 tests pass (165 unit, 833 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).
- **Docs**: Updated README "What the compiler currently supports" blockquote and "Compiler limits" section; removed MAX_SWITCH_LABELS table row; fixed-capacity inventory and self-compilation report updated.
- **Notes**: These are the final two major fixed-capacity bottlenecks in the project (unary-operator buffer and per-switch label cap). All converter candidates from the fixed-capacity inventory are now complete.
