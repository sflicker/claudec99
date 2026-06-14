# stage-122 Transcript: ABI Callee-Saved Register Preservation

## Summary

Stage 122 fixes an ABI violation in the generated code: the `rbx` register is now properly saved at `[rbp - 8]` in every function prologue and restored before every `ret`, satisfying the SysV AMD64 ABI callee-saved register requirement (§3.2.1).

The root cause was that the compiler uses `rbx` as a scratch register for address arithmetic (array indexing, struct member access, lvalue increment/decrement) via a balanced push/pop pattern within expression subtrees. While this pattern is always balanced within a single expression, external code — particularly glibc `qsort` — that calls our generated functions as callbacks relies on `rbx` being preserved across the call, a requirement we violated. The fix reserves a dedicated 8-byte slot at `[rbp - 8]` in every function's stack frame, saves `rbx` there in the prologue, and restores it before each of the four epilogue paths. For variadic functions, an additional +8 alignment pad is inserted before the 176-byte register-save area to preserve 16-byte XMM alignment. All 21 `test/print_asm/` expected files were regenerated to reflect the new prologue/epilogue instructions and shifted local-variable offsets.

## Changes Made

### Step 1: Codegen Stack Layout and Prologue

- `stack_size` now adds +8 to reserve a slot for rbx (was 0 offset).
- `cg->stack_offset` initialization changed from 0 to 8, so the first local variable slot is at `[rbp - 16]` instead of `[rbp - 8]`.
- For variadic functions, before adding 176 (XMM/GP register-save area size) to `stack_offset`, the value is rounded up to the next multiple of 16 to preserve 16-byte alignment for `movaps` instructions on XMM slots.
- After `sub rsp, N` instruction, emit `mov [rbp - 8], rbx` to save rbx immediately after the prologue.

### Step 2: Codegen Epilogue Restoration

- All four epilogue code paths that emit `mov rsp, rbp; pop rbp; ret` sequences now emit `mov rbx, [rbp - 8]` immediately before the `mov rsp, rbp` instruction.
- Ensures rbx is restored to its original value before returning to the caller.

### Step 3: Print-ASM Test Files

- Regenerated all 21 `test/print_asm/*.expected` files to reflect:
  - New `mov [rbp - 8], rbx` instruction in every prologue.
  - New `mov rbx, [rbp - 8]` instruction before each epilogue sequence.
  - Shifted offsets for all local variables (now starting at offset 8 instead of 0 relative to frame layout).

### Step 4: New Tests

- `test/valid/pointers/test_qsort_struct_cmp__0.c`: A callback passed to `qsort` that accesses struct members via the arrow (`->`) operator, exercising `rbx` scratch register usage in a callback context.
- `test/valid/pointers/test_array_index_in_callback__0.c`: A callback passed to `qsort` that uses array subscript indexing, also exercising `rbx` in a callback context.

### Step 5: Version

- `src/version.c`: VERSION_STAGE bumped to `"01220000"`.

## Final Results

All 1894 tests pass:
- 1210 valid tests
- 260 invalid tests
- 88 integration tests
- 50 print-AST tests
- 100 print-tokens tests
- 21 print-asm tests
- 165 unit tests

Self-host verification: C0 (GCC-built) → C1 (self-compiled) → C2 (self-compiled again). All tests pass at each stage with no source code changes required during bootstrap.

## Session Flow

1. Analyzed ABI requirement for callee-saved register preservation.
2. Identified root cause: `rbx` used as scratch register without prologue/epilogue save/restore.
3. Reviewed affected codegen sites: prologue emission, epilogue paths (four locations), and stack layout computation.
4. Implemented prologue changes: stack slot reservation, `rbx` save instruction.
5. Implemented epilogue changes: `rbx` restore instruction before each `mov rsp, rbp`.
6. Updated stack-offset initialization to account for reserved slot.
7. Added alignment rounding for variadic function frame layout.
8. Regenerated all 21 print-asm expected files.
9. Added two new qsort-based callback tests to validate the fix.
10. Verified all 1894 tests pass.
11. Performed self-host C0→C1→C2 bootstrap cycle and confirmed all tests pass at each stage.

## Notes

The compiler's internal use of `rbx` via balanced push/pop pairs is safe for intra-expression temporary storage, but external callers expect `rbx` to be callee-saved across function boundaries per the SysV AMD64 ABI. The fix separates internal temporary use (push/pop within the expression subtree, unchanged) from the required ABI invariant (save/restore in prologue/epilogue, now added). No change to parser, AST, or token stream. The fix is transparent to user code and does not affect the compiler's ability to self-compile.
