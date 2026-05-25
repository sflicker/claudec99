# stage-68 Transcript: Support More Than 6 Arguments in Function Calls

## Summary

Stage 68 removes the hard-coded 6-argument limit from the ClaudeC99 compiler and implements proper System V AMD64 ABI stack-passing for function calls with more than 6 arguments. Previously, the compiler rejected any call with more than 6 arguments at the parser level. Now it generates correct x86-64 code where arguments 1–6 are placed in registers (rdi, rsi, rdx, rcx, r8, r9) and arguments 7+ are pushed on the stack right-to-left. The implementation handles 16-byte stack alignment, type-aware sign/zero-extension of stack parameters in the callee prologue, and both direct and indirect function calls with any argument count.

## Changes Made

### Step 1: Parser

- Removed the hard-coded `child_count > 6` check in `parse_postfix_expression()` that rejected function calls with more than 6 arguments.

### Step 2: Code Generator — Caller Side (Direct Calls)

- Added a macro `EMIT_ARG_CONVERT` to factor out argument type-checking and conversion logic.
- For calls with N ≤ 6 arguments: existing behavior preserved (evaluate all args left-to-right, push all to stack, pop to registers in order, call).
- For calls with N > 6 arguments:
  - Compute alignment padding: `(push_depth + num_stack_args) % 2` determines if one padding word is needed to maintain 16-byte alignment before the call.
  - Push padding word first (if needed).
  - Evaluate and push stack arguments (arguments 6 through N-1) right-to-left, so arg6 lands at [rsp] and becomes [rbp+16] in the callee.
  - Evaluate and push register arguments (arguments 0 through 5) left-to-right to the stack.
  - Pop all 6 register arguments in reverse order (from rax) into rdi, rsi, rdx, rcx, r8, r9.
  - Emit call instruction.
  - Clean up stack with `add rsp, (num_stack_args + padding) * 8`.

### Step 3: Code Generator — Caller Side (Indirect Calls)

- Removed the hard-coded `nargs > 6` rejection.
- For indirect calls with N > 6 arguments:
  - Push the callee address first (deepest on stack).
  - Compute and push padding word if needed.
  - Push stack arguments (6 through N-1) right-to-left.
  - Push register arguments (0 through 5) left-to-right.
  - Pop register arguments into rdi, rsi, rdx, rcx, r8, r9.
  - Retrieve callee address: `mov r10, [rsp + (num_stack_args + padding) * 8]`.
  - Call via `call r10`.
  - Clean up with `add rsp, (num_stack_args + padding + 1) * 8` (the +1 accounts for the saved callee address).

### Step 4: Code Generator — Callee Side

- Removed the hard-coded `num_params > 6` rejection in `codegen_function()`.
- Split parameter handling into two loops:
  - Register parameters (0–5): existing behavior (move from register, no sign/zero extension needed for register-received arguments).
  - Stack parameters (6+): new loop that reads from [rbp + 16 + (i-6)*8], applies sign/zero-extension matching the parameter's declared type and signedness, and stores into the local-variable offset assigned to that parameter.

### Step 5: Tests

- Added 7 new valid tests:
  - `test_seven_arg_call__42.c`: simple function with 7 int parameters, returns the last one.
  - `test_eight_arg_sum__42.c`: function with 8 int parameters, returns their sum.
  - `test_stack_arg_expr__42.c`: stack argument evaluated from an expression (x + 2).
  - `test_stack_arg_pointer__42.c`: stack argument is a pointer, dereferenced in the function.
  - `test_stack_arg_string__1.c`: stack argument is a string literal, compared with strcmp.
  - `test_many_args_mixed_widths__42.c`: 7 parameters of varying types (int, short, char, long, long long, unsigned int, unsigned long long).
  - `test_thirteen_arg_sum__42.c`: function with 13 int parameters, returns their sum.
- Removed 1 invalid test:
  - `test_invalid_19__max_supported_is_6.c`: this test explicitly validated the old 6-argument limit.
- Added 1 integration test:
  - `test_printf_seven_args/`: calls `printf` with 7 format arguments, verifying variadic external calls work with stack arguments.

### Step 6: Documentation

- No grammar changes needed (the language syntax for function calls is unchanged; only code generation behavior changes).

## Final Results

All 1136 tests pass with no regressions.

Suite breakdown:
- valid: 705 tests (added 7 new tests; was 698)
- invalid: 212 tests (removed 1 test; was 213)
- integration: 60 tests (added 1 new test; was 59)
- print_ast: 39 tests (unchanged)
- print_tokens: 99 tests (unchanged)
- print_asm: 21 tests (unchanged)

Total: 1136 (was 1129)

## Session Flow

1. Read spec, templates, and supporting documentation.
2. Reviewed existing parser and code-generation logic for function calls.
3. Identified removal points: hard-coded `child_count > 6` check in parser, `nargs > 6` checks in both direct and indirect call codegen, `num_params > 6` check in callee prologue.
4. Implemented caller-side direct-call stack-passing: padding calculation, right-to-left push of stack args, left-to-right push of register args, pop to registers, cleanup.
5. Implemented caller-side indirect-call stack-passing: similar flow with callee address preservation via r10.
6. Implemented callee-side stack-parameter copying: loop over parameters 6+, read from [rbp + 16 + offset], apply sign/zero-extension, store to local slot.
7. Compiled and verified with test suite.
8. Generated milestone summary and transcript artifacts.

## Notes

- Stack alignment is computed as `(push_depth + num_stack_args) % 2`. This ensures the stack is 16-byte aligned before the call instruction (satisfying the System V AMD64 ABI requirement that rsp % 16 == 0 at the call boundary).
- Stack arguments are always pushed right-to-left so the lowest-indexed stack argument (arg6) lands at the top of the stack and becomes [rbp+16] in the callee frame.
- Indirect calls require temporary storage of the callee address because r10 must be available for other computations during argument evaluation.
- Type-aware sign/zero-extension in the callee prologue ensures that parameters of narrower types (char, short) are correctly extended when loaded from the stack.
