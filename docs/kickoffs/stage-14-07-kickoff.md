# Stage-14-07 Kickoff

## Spec
`docs/stages/ClaudeC99-spec-stage-14-07-libc-puts.md`

## Stage Goal
Verify that the existing function-declaration and function-call
support work end-to-end with libc string functions, beginning with
`puts`. A program of the form

```c
int puts(char *s);

int main() {
    puts("hello");
    return 0;
}
```

must compile, link against libc, run, and exit with code 0.

## Delta from Stage 14-06
The compiler already parses `int puts(char *s);` and accepts a
string literal as a `char *` argument (Stage 14-04 made string
literals decay to `char *` and Stage 12-04 added pointer-typed
parameters). Two pieces are missing:

1. Codegen never emits `extern <name>` directives, so NASM has no
   way to mark `puts` as an undefined symbol that the linker should
   resolve externally.
2. The valid test runner links with `ld -e main`, which does not
   pull in crt0 or libc, so `puts` would be unresolved at link
   time even if `extern` were emitted.

## Ambiguities Flagged
- "Ensure generated assembly links successfully with libc using
  the existing linker path." The existing literal command
  (`ld -e main`) cannot pull in libc without additional flags. The
  cleanest portable interpretation is to keep the same backend
  linker (`ld`) but invoke it via `gcc -no-pie`, which provides
  crt0 and libc. Our existing `mov eax, 60 / syscall` exit
  sequence is compatible with crt0 since it never returns to
  `_start`.
- The spec asks to verify that `"hello"` is output. The valid
  runner only checks exit codes today. Stdout content is verified
  indirectly through a `print_asm` snapshot of the call sequence
  rather than introducing a new stdout-capture mechanism in the
  runner.

## Planned Changes

### Tokenizer (`src/lexer.c`)
- No changes.

### Parser (`src/parser.c`)
- No changes.

### AST
- No changes.

### Code Generator (`src/codegen.c`)
- Add a small helper `function_has_body(ASTNode *func)` that
  inspects the children of an `AST_FUNCTION_DECL` for a trailing
  `AST_BLOCK`.
- In `codegen_translation_unit`, after emitting `section .text`,
  walk the TU children and emit `extern <name>` for each
  `AST_FUNCTION_DECL` that has no body and whose name is not
  defined anywhere in the TU. Dedup so multiple non-defining
  declarations of the same name produce a single `extern`.

### Test Runner (`test/valid/run_tests.sh`)
- Switch the link command from `ld -e main` to `gcc -no-pie`. This
  pulls in crt0 (which calls `main` from `_start`) and libc, while
  leaving the inline exit syscall in `main` unchanged.
- Redirect each binary's stdout to `/dev/null` so tests that
  print don't pollute the runner's output.

### Tests
Valid (`test/valid/`):
- `test_libc_puts_hello__0.c`
- `test_libc_puts_two_calls__0.c`

Snapshot (`test/print_asm/`):
- `test_stage_14_07_libc_puts.c` + `.expected`

### Documentation
- `README.md`: update the "Through stage" line, refresh the test
  counts, and add a note about libc linking.
- `docs/grammar.md`: no changes.

### Commit
Single commit at the end of the stage.
