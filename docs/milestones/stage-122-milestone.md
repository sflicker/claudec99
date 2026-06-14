# Milestone Summary

## Stage 122: ABI Callee-Saved Register Preservation - Complete

stage-122 fixes the generated code to preserve the `rbx` register across function calls, as required by the SysV AMD64 ABI (§3.2.1: `rbx` is callee-saved).

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: Reserved 8-byte stack slot at `[rbp - 8]` for `rbx` in all function prologues and epilogues. `cg->stack_offset` starts at 8 (was 0); `stack_size` adds +8 for the slot; variadic functions add an additional +8 alignment pad before the 176-byte register save area (so XMM slots remain 16-byte aligned); four epilogue paths emit `mov rbx, [rbp - 8]` before `mov rsp, rbp`. All 21 `test/print_asm/` expected files regenerated.
- Tests: 2 new tests added (`test_qsort_struct_cmp__0.c`, `test_array_index_in_callback__0.c`). Full suite 1894/1894 pass (1210 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- Docs: Kickoff artifact generated.
- Notes: Self-host C0→C1→C2 verified with no source changes during bootstrap. The compiler uses `rbx` as a scratch register for address arithmetic via balanced push/pop pairs within expression subtrees; without prologue/epilogue preservation, external callers (e.g., glibc `qsort`) that rely on `rbx` being preserved across a call suffered silent data corruption.
