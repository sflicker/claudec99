# Stage 122 — Callee-Saved `rbx` Preservation

## Summary

Fix the generated code to preserve the `rbx` register across function calls, as required by the SysV AMD64 ABI. The compiler uses `rbx` extensively as a scratch register for address arithmetic (array indexing, struct member assignment). Currently, the prologue never saves `rbx` and the epilogue never restores it, violating the ABI. This causes silent data corruption when external library code (e.g., glibc `qsort`) calls our generated functions as callbacks and expects `rbx` to be preserved.

**This is a codegen-only stage** — no tokenizer, parser, or AST changes.

## Tokenizer changes

None.

## Parser changes

None.

## AST changes

None.

## Code generation changes

All changes are in `src/codegen.c` and `src/version.c`.

### Task 1 — Pre-reserve rbx slot in `stack_size` precomputation (around line 6169)

Add `+8` to `stack_size` for the rbx save slot at `[rbp - 8]`. For variadic functions, add `+8` alignment padding before adding `+176` for the register save area (reason: `stack_offset` now starts at 8, so variadic save area needs padding to align to 16 bytes for `movaps`).

**Frame layout after stage 122:**
```
[rbp]       — saved caller rbp
[rbp -  8]  — RESERVED: saved rbx
[rbp - 12]  — first 4-byte local variable
...
```

### Task 2 — Initialize `cg->stack_offset` to 8 (around line 6122)

Change `cg->stack_offset = 0` to `cg->stack_offset = 8` to reserve `[rbp - 8]` for rbx.

### Task 3 — Align variadic save area offset (around line 6211)

Before `cg->stack_offset += 176`, round `cg->stack_offset` to the next multiple of 16 to ensure XMM slots remain properly aligned. With offset=8, this pads to 16 (+8 bytes) then adds 176 = 192 (≡ 0 mod 16).

### Task 4 — Save rbx in prologue (after line 6193)

After `sub rsp, N`, emit: `mov [rbp - 8], rbx` to save the callee-saved register into the reserved slot.

### Task 5 — Restore rbx in all four epilogue paths

Four locations in `src/codegen.c` emit `pop rbp; ret`. In each, add `mov rbx, [rbp - 8]` immediately before `mov rsp, rbp`:

- **Epilogue A** (around line 5745): bare `return;` from void function
- **Epilogue B** (around line 5785): struct-by-value return
- **Epilogue C** (around line 5844): normal `return <expr>;`
- **Epilogue D** (around line 6457): implicit fall-off-end for void functions

### Task 6 — Bump version (src/version.c)

Change `VERSION_STAGE` to `"01220000"`.

## Test plan

1. **Existing regression test**: `test/valid/stdlib/test_stdlib_qsort__0.c` (uses qsort with comparator that dereferences pointers, triggering rbx scratch).

2. **New tests in `test/valid/pointers/`**:
   - `test_qsort_struct_cmp__0.c` — qsort comparator using struct arrow access (exercises rbx scratch via `->` member access)
   - `test_array_index_in_callback__0.c` — qsort comparator using array subscript (exercises rbx scratch via subscript indexing)

3. **Test regeneration**: After C1 passes all valid/invalid suites, regenerate all `test/print_asm/*.expected` files (every function's prologue/epilogue changes).

## Implementation order

1. Apply Tasks 1–5 in codegen.c (stack_size, stack_offset init, variadic alignment, prologue save, four epilogue restores)
2. Build with cmake
3. Smoke test: run `test/valid/stdlib/test_stdlib_qsort__0.c` (must exit 0)
4. Run `test/valid/run_tests.sh` — all existing tests must pass
5. Add new tests; run full suite
6. Bump `src/version.c`
7. Self-host: `./build.sh --mode=self-host` — all three passes (C0→C1→C2) must pass
8. Update `docs/self-compilation-report.md`
9. Commit

## Bootstrap caveats

This stage changes the assembly output of **every compiled function**: the prologue gains `mov [rbp - 8], rbx` and each epilogue gains `mov rbx, [rbp - 8]`. All local variable offsets increase by 8 because `cg->stack_offset` now starts at 8.

- **C0 → C1**: C0 (GCC-compiled, no rbx save) compiles the source into C1 which DOES save/restore rbx. C1's assembly differs from C0's for every function (expected and correct).
- **C1 → C2**: C1 compiles the same source into C2. C2's assembly must match C1's instruction-for-instruction (both apply the same rbx-save scheme) — key self-host invariant.
- **Test suite**: All 1879+ existing tests must pass at C0, C1, and C2. Behavioral contract is unchanged; only generated code structure changes.

## Ambiguities / Issues

None. Spec is clear and complete. The variadic alignment math is well-explained (offset 8 → round to 16 → add 176 → offset 192, which is 0 mod 16).
