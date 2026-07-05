# Stage-175 Kickoff

## Spec Summary

Stage 175 is a bugfix for a parser type-construction defect affecting function parameters declared as a combination of pointer stars and array suffix (e.g., `char *argv[]`). Per C99 6.7.5.3p7, an array parameter decays to a pointer to its element type. When a parameter has both a declared pointer star (`*`) and a trailing array-suffix (`[]`), the parser was incorrectly computing the decayed type, causing downstream type-checking errors. The specification documents a real-world networking program that failed to compile with the error `expected pointer argument, got integer` on a call like `getaddrinfo(argv[1], NULL, &hints, &res)` — even though the program compiles fine with gcc using real system headers. Through bisection, the root cause was narrowed to this parser parameter-type bug: `char *argv[]` was being recorded as `char *` instead of the correct `char **` after array decay, so subscripting `argv[1]` produced an `int` (plain `char`) instead of a pointer (`char *`).

## What changes from stage 174

No tokenizer, lexer, AST node, or codegen changes are needed. This is purely a parser type-construction fix in `parse_parameter_declaration()` in `src/parser.c`.

**Parser fix:**
- Reorder the branch chain in `parse_parameter_declaration()` so the `d.is_array` check runs first (ahead of `d.pointer_count > 0`), ensuring array decay is applied on top of any declared pointer levels.
- When `d.is_array` is true, apply one extra `type_pointer()` level on top of the declared stars to yield the correct decayed type.
- The pure-array case (`int a[3]`, zero declared stars) still produces the same result as before.

## Root Cause Analysis

The bug occurred because in the original code, an `if (d.pointer_count > 0)` branch ran before the `else if (d.is_array)` branch. This meant whenever a declared pointer star existed, the first branch would claim the declaration and use only the declared stars, discarding any array decay logic. So `char *argv[]` (which has `d.pointer_count == 1` and `d.is_array == true`) would take the first branch and yield `char *` instead of applying the decay rule to produce `char **`.

## Test Plan

A new portable integration test `test_array_param_pointer_arg` will be added to isolate the defect without requiring system headers. The test defines a function `second_arg_len(int argc, char *argv[])` that subscripts `argv[1]` (which must have type `char *` after decay) and passes the result to `str_len(char *s)` — the exact type-checking pattern that previously mis-flagged. A local array of pointers `char *args[] = {"prog", "hello"}` is passed to `second_arg_len`, and the exit status is expected to be 5 (the length of `"hello"`). This confirms both that the type-check no longer false-flags and that the runtime subscript arithmetic is correct.

Note: The original spec's `getaddrinfo` / system-header repro was not ported to a portable test because it requires system networking headers (`--sysinclude`) and doesn't actually need network access at compile-time (only up through argument type-checking). The new minimal portable test isolates the actual defect — the parser's array-decay computation — without the system-header and network dependencies.

## Files Changed This Stage

- `src/parser.c` — function `parse_parameter_declaration()`, reorder branches for correct array decay on pointer-and-array-suffix parameters.
- `src/version.c` — version bump from `01740000` to `01750000`.
- `docs/outlines/checklist.md` — new Stage 175 section.
- `test/integration/test_array_param_pointer_arg/test_array_param_pointer_arg.c` (new).
- `test/integration/test_array_param_pointer_arg/test_array_param_pointer_arg.status` (new).
- `docs/self-compilation-report.md` — new stage-175 entry (already written).

## Test Results Expected

Full suite should pass with 2075 portable tests (165 unit, 1286 valid, 261 invalid, 192 integration [+1 new], 50 print-AST, 100 print-tokens, 21 print-asm), 192 system-include, 2 optional-library, 5 build-tool. Zero regressions. Self-host C0→C1→C2 cycle should complete successfully with no source changes required.
