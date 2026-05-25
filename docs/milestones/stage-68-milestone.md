# Milestone Summary

## Stage 68: Support More Than 6 Arguments in Function Calls - Complete

stage-68 removes the hard-coded 6-argument limit and implements full System V AMD64 ABI stack-passing for function calls with 7+ integer, pointer, _Bool, enum, and struct-pointer arguments.

- Tokenizer: No changes.
- Parser: Removed hard-coded `child_count > 6` limit check that rejected function calls exceeding 6 arguments.
- AST/Semantics: No changes.
- Codegen (caller side): For calls with N ≤ 6 args, behavior unchanged (all args in registers). For N > 6: compute 16-byte stack alignment padding, evaluate and push stack args (args 6..N-1) right-to-left, evaluate and push register args (args 0..5) left-to-right, pop register args to rdi/rsi/rdx/rcx/r8/r9, call, then clean up with `add rsp, (num_stack_args + padding) * 8`. Indirect calls follow the same strategy with callee address saved/restored via r10.
- Codegen (callee side): Removed hard-coded `num_params > 6` limit. Stack parameters (7+) are copied from [rbp + 16 + (i-6)*8] into local stack slots in the prologue with automatic sign/zero extension matching the parameter's type and signedness.
- Tests: Added 7 new valid tests (seven/eight/thirteen argument calls, stack args with expressions/pointers/strings, mixed-width arguments) and 1 integration test (printf with 7 arguments); removed 1 invalid test (the old 6-arg limit check). Full suite 1136/1136 pass (705 valid, 212 invalid, 60 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- Docs: README.md updated with stage 68 milestone, call-capability statement, test counts, and "not yet supported" list cleaned of 6-arg limit. No grammar changes needed.
- Notes: All stack-passing respects System V AMD64 alignment requirements (16-byte before call, 8-byte after push of return address).
