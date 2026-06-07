# Milestone Summary

## Stage 95-07: Fixed-Capacity Conversions (Switch + Call Layout) - Complete

stage-95-07 converts two remaining fixed-capacity items in the code generator with lowest blast radius: switch depth tracking and call argument bounds checking.

- **Tokenizer**: No changes.
- **Parser**: No changes.
- **AST**: No changes.
- **Codegen**: Two changes:
  1. `MAX_SWITCH_DEPTH` converted: `CodeGen.switch_stack[16]` replaced with `Vec switch_stack`. The switch statement handler pushes/pops via `vec_push`/`vec_pop`, eliminating the hard depth cap.
  2. `MAX_CALL_LAYOUT_ITEMS` bounds check: A `compile_error` guard added to `compute_call_layout` to catch >24 arguments at compile time, replacing silent stack overflow risk.
- **Tests**: All 1471 tests pass (existing suite, no new tests). Bootstrap verification: C0→C1→C2 all pass with no failures.
- **Docs**: README updated to remove MAX_SWITCH_DEPTH from Code generator limits table. Fixed-capacity inventory updated (MAX_SWITCH_DEPTH marked DONE; remaining deferred items documented with rationale). Self-compilation report updated with C2 version.
- **Notes**: Both changes follow the spec's lowest-blast-radius guidance. MAX_CALL_LAYOUT_ITEMS remains non-Vec by design (local stack variable, no meaningful gain). Structural-refactoring items (FUNC_MAX_PARAMS, FUNC_TYPE_MAX_PARAMS, MAX_SWITCH_LABELS, MAX_USER_LABELS, MAX_NAME_LEN, MAX_ARRAY_DIMS, MAX_INCLUDE_DEPTH, MAX_COND_DEPTH) deferred to future stages.
