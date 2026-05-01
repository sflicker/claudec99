# stage-14-07 Transcript: Calling libc `puts`

## Summary

Stage 14-07 lets the compiler produce programs that call libc
functions declared at the top of the translation unit, beginning
with `puts`. Function-declaration parsing, function-call codegen,
pointer-typed parameters, and string-literal decay to `char *` were
all already in place from prior stages; this stage closes the gap
by emitting `extern <name>` for every declared-but-not-defined
function so the linker resolves the symbol against libc, and by
switching the valid test runner from bare `ld -e main` to
`gcc -no-pie` so crt0 and libc are linked in.

Two ABI-related codegen adjustments fall out of the linker change.
The function prologue is now always emitted (`push rbp; mov rbp,
rsp`), so rsp is 16-byte aligned at every internal call site as
SysV AMD64 requires for libc routines that use SSE. The
`sub rsp, N` is still elided when there are no locals. `main`'s
exit path becomes a normal `ret` instead of a direct
`mov eax, 60; syscall`, so `__libc_start_main` calls libc `exit`
and the stdio cleanup path runs — without this, buffered `puts`
output to a non-TTY stdout would be silently dropped.

Forward-declared functions that are also defined in the same
translation unit are not externed.

## Plan

The plan paralleled the spec's verification framing: emit
`extern` for declared-but-not-defined functions, switch the test
runner's linker, and add a runtime test plus a snapshot. ABI
fixes (always-emit prologue, ret-from-main) emerged during the
first puts test run when the binary segfaulted at `call puts` and
then exited cleanly but produced no output.

## Changes Made

### Step 1: Code Generator — extern emission

- `src/codegen.c`: added `function_has_body`,
  `tu_has_definition_for`, and `codegen_emit_externs` helpers.
  `codegen_translation_unit` now walks the TU children before
  emitting any function and prints `extern <name>` for each
  `AST_FUNCTION_DECL` whose name has no defining counterpart
  anywhere in the TU. Multiple non-defining declarations of the
  same name collapse to a single `extern`.

### Step 2: Test Runner

- `test/valid/run_tests.sh`: link command switched from
  `ld -e main` to `gcc -no-pie`. Each binary's stdout is
  redirected to `/dev/null` during the exit-code check so test
  output stays line-oriented.

### Step 3: ABI — always-emit prologue

- `src/codegen.c`: function prologue is now always emitted
  (`push rbp; mov rbp, rsp`). Under gcc-linked execution
  `_start` calls `main`, so rsp is 8 mod 16 at main entry; the
  unconditional push rbp re-establishes 16-byte alignment that
  SysV AMD64 requires at every internal call site (notably libc
  calls such as `puts` that use SSE-aligned memory ops).
  `sub rsp, N` is still elided when there are no locals.

### Step 4: ABI — ret-from-main

- `src/codegen.c`: `main`'s exit path changed from
  `mov edi, eax; mov eax, 60; syscall` to the same epilogue used
  by every other function (`mov rsp, rbp; pop rbp; ret`).
  Returning normally lets `__libc_start_main` call libc `exit`,
  which runs stdio cleanup and flushes buffered output — without
  this, `puts("hello")` output to a piped/redirected stdout was
  silently lost.

### Step 5: Tests

Valid runtime tests under `test/valid/`:

- `test_libc_puts_hello__0.c`
- `test_libc_puts_two_calls__0.c`

Snapshot test under `test/print_asm/`:

- `test_stage_14_07_libc_puts.c` + `.expected`

Refreshed snapshots (every `.expected` under
`test/print_asm/` updated to reflect the always-prologue and
ret-from-main changes):

- `test_stage_14_03_string_literal_basic.expected`
- `test_stage_14_03_string_literal_empty.expected`
- `test_stage_14_03_string_literal_one.expected`
- `test_stage_14_05_string_literal_escape_n.expected`
- `test_stage_14_05_string_literal_escape_null.expected`
- `test_stage_14_06_char_array_string_init_explicit.expected`
- `test_stage_14_06_char_array_string_init_inferred.expected`

### Step 6: Documentation

- `README.md`: through-stage line, functions bullet, end-to-end
  build/run example, and aggregate test counts updated.
- `docs/grammar.md`: no changes.

## Final Results

Build clean. 405 / 405 tests pass (252 valid + 49 invalid + 23
print-AST + 73 print-tokens + 8 print-asm). No regressions.

## Session Flow

1. Read the Stage 14-07 spec and supporting templates / notes.
2. Reviewed function-declaration parsing, function-call codegen,
   string-literal decay, and the existing test runner.
3. Produced a kickoff with delta from Stage 14-06, ambiguities,
   and a step-by-step plan; saved under `docs/kickoffs/`.
4. Implemented `extern` emission and switched the runner to
   `gcc -no-pie`; rebuilt and confirmed the existing 402 tests
   still pass.
5. Authored the puts tests and the print_asm snapshot; the first
   run revealed a segfault at `call puts` (rsp misaligned) and a
   silent-no-output case (stdio not flushed).
6. Made the always-emit-prologue change to fix the alignment
   issue; made the ret-from-main change to let `__libc_start_main`
   flush stdio at exit.
7. Refreshed every print_asm snapshot to reflect the new
   prologue/exit shape.
8. Verified `puts` actually prints by running the generated
   binary directly and capturing stdout.
9. Updated `README.md` and recorded the milestone + transcript.

## Notes

- "Existing linker path" in the spec was read as "the linker
  tooling on the system" rather than the literal current
  invocation. `gcc -no-pie` keeps the same backend `ld` while
  pulling in crt0 and libc; the spec's stated goal of linking
  successfully against libc is met.
- Stdout output from `puts` was verified by running the
  generated binary directly. The valid runner only checks exit
  codes; the print_asm snapshot is the structural verification
  that the call sequence (`extern puts`, `lea rdi, [rel Lstr0]`,
  `call puts`) is emitted correctly.
- The always-emit-prologue change adds 2 instructions to any
  function that previously had `stack_size == 0`. None of the
  existing `print_asm` snapshots were in that bucket, so only
  the `main`'s exit path changed in those snapshots; the new
  puts snapshot is the only file that exercises both the new
  prologue and `extern puts`.
