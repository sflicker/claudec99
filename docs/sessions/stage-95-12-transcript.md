# stage-95-12 Transcript: Fix #if Unary Overflow and Dynamic Switch Labels

## Summary

Stage 95-12 completes two final fixed-capacity conversions in the compiler. The preprocessor's `eval_cond_unary` function used a fixed 32-element `char ops[]` buffer to collect unary operators (`!`, `-`, `+`, `~`) in `#if`/`#elif` expressions; chains longer than 32 would overflow and SIGSEGV. This is replaced with a dynamic `StrBuf`, growing on demand. Simultaneously, the code generator's `SwitchCtx` converted from parallel fixed arrays to a dynamic `Vec`, eliminating the hard limit of 256 case/default labels per switch. Both changes remove unbounded buffer-overflow risks and static capacity constraints from the compiler's remaining table-based structures.

## Changes Made

### Step 1: Preprocessor Unary-Operator Buffer

- Included `strbuf.h` in src/preprocessor.c.
- Replaced the local `char ops[32]` array in `eval_cond_unary` with a dynamic `StrBuf ops`.
- Initialization: `strbuf_init(&ops)` at function entry (the buffer allocates lazily; no initial-capacity argument).
- Each consumed unary operator (when peeking `!`, `-`, `+`, `~`) is now appended via `strbuf_append_char(&ops, c)`.
- Loop index changed from iterating a fixed-size array to walking `ops.data[i]` in reverse over `ops.len`, applying operators right-to-left.
- Cleanup: `strbuf_free(&ops)` before returning the folded value.
- Unary chains of arbitrary length now supported; no overflow.

### Step 2: Code Generator SwitchCtx Dynamic Vec

- Added `typedef struct { ASTNode *node; int label; } SwitchLabel;` to include/codegen.h.
- Converted `SwitchCtx` from:
  ```c
  ASTNode *nodes[MAX_SWITCH_LABELS];
  int labels[MAX_SWITCH_LABELS];
  int count;
  ```
  to:
  ```c
  Vec entries; /* SwitchLabel */
  int default_label;
  ```
- Removed `MAX_SWITCH_LABELS` from include/constants.h.
- Updated `collect_switch_labels` in src/codegen.c: dropped both overflow guards (`>= MAX_SWITCH_LABELS`), now calls `vec_push(&ctx->entries, &label_entry)` for each case.
- Updated `switch_lookup_label`: replaced direct array access with `vec_get(&ctx->entries, i)` iteration.
- Updated AST_SWITCH_STATEMENT handler: changed dispatch loop to iterate over `vec_get(&ctx->entries, ...)`.
- Fixed critical bug: post-body cleanup originally called `vec_free(&ctx->entries)` using the old stack-frame `ctx` pointer, which was stale after nested-switch processing reallocated the `switch_stack`. Fixed by re-fetching the live top element: `SwitchCtx *live_ctx = vec_get(&cg->switch_stack, cg->switch_stack.len - 1); vec_free(&live_ctx->entries);`.
- Lifecycle: `vec_init(&new_ctx.entries, sizeof(SwitchLabel))` before any `vec_push`; `vec_free(&live_ctx->entries)` after body emission, before `vec_pop`.

### Step 3: Version and Tests

- Bumped VERSION_STAGE in src/version.c to `"00951200"`.
- Added `test/valid/test_pp_if_long_unary_chain__42.c`: 50 pairs of `-` and `~` (100 operators) applied to 0, folded right-to-left to exactly 50; `#if (expr) == 50` selects the return-42 branch. Proves no buffer overflow and correct right-to-left associativity.
- Added `test/valid/test_switch_over_256_labels__77.c`: a 300-case `switch` (0..299); case 277 (exceeding old 256 limit) returns 77. Proves per-switch label cap is gone and correct case selection at runtime.

### Step 4: Documentation

- Updated docs/fixed-capacity-inventory.md: marked preprocessor unary-operator buffer and MAX_SWITCH_LABELS as completed.
- Updated docs/self-compilation-report.md: confirmed all tests pass at C0, C1, C2 bootstrap stages.

## Final Results

All 1481 tests pass at GCC build and at C0, C1, C2 self-host bootstrap:
- Unit: 165 assertions
- Valid: 833 cases
- Invalid: 237 cases
- Integration: 82 suites
- Print-AST: 43 cases
- Print-tokens: 100 cases
- Print-asm: 21 cases

No regressions. Bootstrap cycle from C0→C1→C2 completes cleanly with no fixes required.

## Session Flow

1. Read stage spec and supporting inventory documentation.
2. Reviewed preprocessor's `eval_cond_unary` logic and SwitchCtx structures in codegen.h.
3. Planned Task 1 conversion (StrBuf for unary ops) and Task 2 conversion (Vec for switch labels).
4. Implemented preprocessor change, tested compilation.
5. Implemented codegen change, discovered and fixed nested-switch dangling-pointer bug via failing `test_switch_nested_20_deep`.
6. Added two new test cases (long unary chain, >256-label switch).
7. Ran full test suite at GCC build (all 1481 pass).
8. Ran self-host bootstrap C0→C1→C2 (all tests pass at each stage).
9. Generated milestone, transcript, and README updates.

## Notes

**Design Decisions:**

- **StrBuf over recursion**: The unary-operator chain could have been handled via recursive descent, but appending to a StrBuf and then iterating right-to-left is simpler and avoids stack growth.
- **Pair-struct Vec over parallel arrays**: Consolidating `(ASTNode*, int label)` into a single `SwitchLabel` struct simplifies the Vec iteration and makes the memory layout explicit; parallel arrays invite index-mismatch bugs.
- **Dangling-pointer fix**: The bug occurred because `ctx` was a local pointer to an element in `cg->switch_stack`, which is reallocated when nested switches are processed. Re-fetching via `vec_get` ensures the pointer is live at the time of `vec_free`.

**Spec Compliance:**

Both Task 1 (unary ops) and Task 2 (switch labels) are complete as specified. No changes to grammar or language semantics. The compiler still rejects unary-operator chains that overflow integer evaluation (e.g., `!!!!!!!!!!...` applied to a large unsigned value flips between 0 and 1), but now without a fixed buffer limit.

**Final Artifact State:**

- src/preprocessor.c, include/codegen.h, src/codegen.c: logic and data structure changes.
- include/constants.h: `MAX_SWITCH_LABELS` removed.
- src/version.c: version bumped.
- test/valid/: two new test cases added.
- docs/fixed-capacity-inventory.md, docs/self-compilation-report.md: updated completion status.
- README.md: updated "What the compiler currently supports" blockquote and "Compiler limits" section.
