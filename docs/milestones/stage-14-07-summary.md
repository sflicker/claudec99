# Milestone Summary

## Stage-14-07: Calling libc `puts` — Complete

The compiler can now produce programs that call libc functions
declared at the top of the translation unit. The motivating
example is `int puts(char *s);` followed by `puts("hello");` from
`main`. Codegen emits `extern <name>` for every function that is
declared but not defined in the translation unit; the test runner
links via `gcc -no-pie` so crt0 and libc are pulled in
automatically.

## Behavior gained
- `int puts(char *s); int main() { puts("hello"); return 0; }`
  compiles, links, runs, prints `hello`, and exits 0.
- Multiple successive libc calls work
  (`puts("A"); puts("B");`).
- Forward-declared functions that are also defined locally are
  not externed (no spurious `extern` lines).

## Code changes
- `src/codegen.c`:
  - New `function_has_body`, `tu_has_definition_for`, and
    `codegen_emit_externs` helpers; `codegen_translation_unit`
    now emits `extern <name>` for each declared-but-not-defined
    function before walking the function bodies.
  - Function prologue is now always emitted (`push rbp; mov rbp,
    rsp`), so rsp is 16-byte aligned at every internal call site
    (required by SysV AMD64 for libc routines that use SSE).
    `sub rsp, N` is still elided when there are no locals.
  - `main`'s exit path changed from `mov eax, 60; syscall` to a
    normal `ret`, so `__libc_start_main` calls libc `exit`, which
    flushes stdio buffers — without this, buffered `puts` output
    to a non-TTY stdout is lost.

- `test/valid/run_tests.sh`: link command switched from
  `ld -e main` to `gcc -no-pie`; binaries' stdout is redirected
  to `/dev/null` during exit-code checks so test output stays
  clean.

## Test changes
- `test/valid/`: added `test_libc_puts_hello__0.c` and
  `test_libc_puts_two_calls__0.c`.
- `test/print_asm/`: added `test_stage_14_07_libc_puts.c` +
  `.expected`. Refreshed all seven prior `.expected` files to
  reflect the new always-prologue and `ret`-from-main exit
  shape.

## Documentation
- `README.md`: through-stage line, functions bullet,
  end-to-end-build-and-run example, and aggregate test counts
  updated.
- `docs/grammar.md`: no changes (no grammar update for this
  stage).

## Build & tests
Build clean. 405 / 405 tests pass (252 valid + 49 invalid + 23
print-AST + 73 print-tokens + 8 print-asm). No regressions.
