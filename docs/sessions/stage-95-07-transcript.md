# stage-95-07 Transcript: Fixed-Capacity Conversions (Switch + Call Layout)

## Summary

stage-95-07 addresses two remaining fixed-capacity limits in the code generator, targeting the lowest blast radius items. The first conversion replaces the hard-coded switch depth array (`MAX_SWITCH_DEPTH=16`) with a dynamic Vec, eliminating a theoretical nesting limit. The second change adds a compile-time bounds check to `compute_call_layout` to prevent silent stack overflow on functions with >24 arguments. Both changes align with the staged fixed-capacity conversion strategy and require no grammar or AST modifications.

## Changes Made

### Step 1: Update Codegen Header

- Changed `CodeGen.switch_stack` field from fixed array `SwitchCtx[16]` to `Vec switch_stack`.
- No structural changes to SwitchCtx itself.

### Step 2: Update Codegen Init

- Modified `codegen_init` to call `vec_init(&codegen->switch_stack)`.

### Step 3: Update Switch Statement Handler

- Modified `AST_SWITCH_STATEMENT` case in `emit_ast`:
  - Creates a local `SwitchCtx` struct on the stack.
  - Calls `vec_push(&codegen->switch_stack, &ctx)` to track the context.
  - Increments `switch_depth` counter.
  - Retrieves the active SwitchCtx pointer via `vec_get` before emitting the body.
  - Calls `vec_pop` after body emission to restore the previous state.

### Step 4: Update Case/Default Handlers

- Modified `AST_CASE_SECTION` and `AST_DEFAULT_SECTION` cases:
  - Both now use `vec_get(&codegen->switch_stack, switch_depth - 1)` to re-acquire the current SwitchCtx pointer instead of direct array indexing.

### Step 5: Remove Fixed Constant

- Removed `MAX_SWITCH_DEPTH` from `include/constants.h`.

### Step 6: Add Call Layout Bounds Check

- Added a `compile_error` guard at the top of `compute_call_layout` in `src/codegen.c`.
- Guard checks if the call has >24 arguments (MAX_CALL_LAYOUT_ITEMS) and reports a compile error if exceeded.
- The `CallLayout` local struct itself remains unchanged; no Vec conversion performed.

### Step 7: Update Version

- Updated `src/version.c` to version `00950700` to mark the stage completion.

### Step 8: Build and Test

- Built with GCC and ran the full 1471-test suite.
- All tests pass with no failures or regressions.

### Step 9: Bootstrap Self-Compilation

- C0 compiled with GCC: version `00.02.00950700.00718/GNU_13_3_0` (1471 tests pass).
- C1 compiled from C0: version `00.02.00950700.00719` (1471 tests pass).
- C2 compiled from C1: version `00.02.00950700.00720` (1471 tests pass).
- All three bootstrap stages pass without errors.

### Step 10: Commit and Document

- Committed all changes with appropriate trailers.
- Updated milestone summary and this transcript.
- Updated fixed-capacity inventory and self-compilation report.

## Final Results

All 1471 tests pass at GCC build, C0, C1, and C2 bootstrap stages. No regressions detected. Bootstrap chain (C0→C1→C2) fully verified with no compilation or test failures.

## Session Flow

1. Read stage spec and reviewed fixed-capacity inventory status
2. Analyzed switch depth array and call layout bounds check requirements
3. Reviewed codegen header and implementation files
4. Updated `include/codegen.h` to change switch_stack to Vec
5. Modified `codegen_init`, switch statement handler, and case/default handlers
6. Added bounds check to `compute_call_layout`
7. Removed MAX_SWITCH_DEPTH from constants
8. Built and ran full test suite (1471 tests)
9. Verified bootstrap chain C0→C1→C2
10. Updated documentation and committed

## Notes

**Design Decisions:**
- **MAX_SWITCH_DEPTH conversion to Vec**: Chosen because switch depth tracking is inherently dynamic and Vec provides the safest path to elimination. The change is localized to the emit_ast switch statement handler.
- **MAX_CALL_LAYOUT_ITEMS bounds check (not Vec)**: The CallLayout local struct is ephemeral (stack variable, no persistent state), so Vec conversion would add overhead without benefit. A simple compile_error guard is the appropriate minimal safety fix.
- **Deferred structural items**: FUNC_MAX_PARAMS, FUNC_TYPE_MAX_PARAMS, MAX_SWITCH_LABELS, MAX_USER_LABELS, MAX_NAME_LEN, MAX_ARRAY_DIMS, MAX_INCLUDE_DEPTH, MAX_COND_DEPTH all require embedding changes in larger data structures (FuncType, Scope, Name, ArrayType, etc.) and carry higher blast radius. These are deferred to future stages as per the spec's guidance.

**Files Changed:**
- `include/codegen.h`
- `include/constants.h`
- `src/codegen.c`
- `src/version.c`
- `docs/fixed-capacity-inventory.md`
- `docs/self-compilation-report.md`
- `README.md`
