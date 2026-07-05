# stage-175 Transcript: Fix char *argv[] Parameter Array Decay

## Summary

Stage 175 fixes a critical parser bug where function parameters declared with both a pointer star and array suffix (e.g., `char *argv[]`) were incorrectly adjusted to the wrong type after array decay. A real-world networking program using `getaddrinfo(argv[1], NULL, &hints, &res)` failed to compile with the error `expected pointer argument, got integer` on the subscript `argv[1]`, even though the program compiles fine with gcc. Through bisection with minimal repro programs, the bug was narrowed to `parse_parameter_declaration()` in `src/parser.c`, which applies C99 6.7.5.3p7 array-to-pointer decay.

The root cause: the parser had an `if (d.pointer_count > 0)` branch that ran before the `else if (d.is_array)` branch. When both conditions were true (declared star plus array suffix), the first branch would claim the parameter and use only the declared stars, discarding the array decay logic. So `char *argv[]` (element type `char *`, which should decay to `char **`) was being recorded as plain `char *`. Subscripting such a parameter then yielded the wrong type, causing downstream type-checking to reject valid function calls.

The fix is one-function: reorder the branch chain in `parse_parameter_declaration()` to check `d.is_array` first. When an array suffix exists, apply one extra `type_pointer()` level on top of the declared pointer levels, correctly implementing array decay on top of declared stars. This is a parser-only change with no tokenizer, lexer, AST, or codegen impact.

## Changes Made

### Step 1: Parser

- `src/parser.c`, function `parse_parameter_declaration()` (around line 4090): reordered the branch chain so the `d.is_array` check runs ahead of `d.pointer_count > 0`.
- When `d.is_array` is true, build the full type by first applying declared stars to base_type, then applying one additional `type_pointer()` level: `full_type = type_pointer(...)` with the accumulated pointer levels, then `param->full_type = type_pointer(full_type)`.
- This ensures `char *argv[]` yields `char **` (pointer to pointer to char) as the correct decayed parameter type.
- The pure-array case (`int a[3]`, zero declared stars) continues to produce the same result as before, since `full_type == base_type` when `pointer_count == 0`.

### Step 2: Version

- `src/version.c`: `VERSION_STAGE` incremented from `"01740000"` to `"01750000"`.

### Step 3: Checklist

- `docs/outlines/checklist.md`: new `## Stage 175 - Fix char *argv[] Parameter Array Decay` section inserted immediately before the `## TODO` marker (after Stage 174), following the exact style and format of the Stage 174 section above it (checked items, "TODO items completed this stage: None (bugfix)" note).

### Step 4: Tests

- New integration test: `test/integration/test_array_param_pointer_arg/test_array_param_pointer_arg.c`. The test defines `second_arg_len(int argc, char *argv[])` which subscripts `argv[1]` and passes the result to `str_len(char *s)` — the exact pattern that previously mis-typechecked. A helper `str_len` counts characters in a pointer. `main` constructs a local `char *args[] = {"prog", "hello"}` and calls `second_arg_len(2, args)`, expecting exit status 5 (the length of "hello").
- Companion status file: `test/integration/test_array_param_pointer_arg/test_array_param_pointer_arg.status` containing `5`.
- This portable test isolates the parser defect without requiring system headers (`--sysinclude`) or the full `getaddrinfo` / system-networking stack from the original spec.

## Final Results

Full test suite via `./test/run_all_tests.sh`:
- Portable: 2075 passed (165 unit, 1286 valid, 261 invalid, 192 integration [+1 new], 50 print-AST, 100 print-tokens, 21 print-asm).
- System-include: 192 passed.
- Optional-library: 2 passed.
- Build-tool: 5 passed.
- Total: 2274 passed, 0 failed, 0 regressions.

Manual smoke test: subscripting a `char *argv[]` parameter now correctly yields type `char *` (pointer), not `char` (integer).

Self-host bootstrap via `./build.sh --mode=self-host`: C0→C1→C2 cycle completed successfully. All stages built as version `00.03.01750000.0127{9,80,81}` respectively, all passing the full test suite with zero source changes required during bootstrap. No regressions.

## Session Flow

1. Read stage 175 spec and supporting documentation (bug report, root cause, test summary).
2. Reviewed `parse_parameter_declaration()` in `src/parser.c` to understand the current branch structure and the bug.
3. Identified the branch ordering issue: `if (d.pointer_count > 0)` before `else if (d.is_array)`.
4. Designed the fix: reorder branches and apply array decay on top of declared pointer levels.
5. Implemented the parser fix in `parse_parameter_declaration()`.
6. Updated `src/version.c` version from `01740000` to `01750000`.
7. Added new Stage 175 section to `docs/outlines/checklist.md` in the appropriate location.
8. Reviewed the existing integration test and confirmed it isolates the parser defect.
9. Ran full test suite: all 2274 tests pass, no regressions.
10. Executed `./build.sh --mode=self-host` for full C0→C1→C2 bootstrap verification; all stages passed with zero source changes.
11. Confirmed `docs/self-compilation-report.md` already contains the stage-175 entry (written during the bootstrap checkpoint commits).
12. Generated milestone summary and transcript artifacts.

## Notes

### Design Decision: Minimal Portable Test vs. Exact Spec Repro

The specification documents a real-world networking program using `getaddrinfo` from `<netdb.h>` that failed to compile with cc99 but works with gcc. The root cause analysis identified the parser bug as the actual defect. Rather than porting the full `getaddrinfo` repro to a portable test (which would require system headers and system-include infrastructure), the stage added a minimal portable test `test_array_param_pointer_arg` that isolates the parser defect in isolation: it exercises the same type-checking pattern (subscripting a `char *argv[]` parameter and passing the result to a function expecting `char *`) without the system-header and network-layer dependencies. This approach keeps the test suite portable and maintainable while confirming that the parser bug is fixed.

### Implementation Notes

The original spec's error message ("expected pointer argument, got integer") was a correct diagnostic given the (wrong) type the parser had computed for `argv[1]`. After the fix, the type is computed correctly and the function call type-checks successfully.

The fix preserves all existing behavior for pure-array parameters (`int a[3]`) and pure-pointer parameters (`int *p`), only correcting the combined "pointer star + array suffix" case (`char *argv[]`) to match C99 §6.7.5.3p7.
