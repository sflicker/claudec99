# stage-14-07-01 Transcript: Optional stdout comparison in valid tests

## Summary

Stage 14-07 added the first valid tests that exercise libc `puts`
but the `valid` runner only checked exit codes — there was no way
to assert that the program actually printed the expected bytes.
This sub-stage extends both the per-suite runner
(`test/valid/run_tests.sh`) and the single-test driver
(`test/valid/run_test.sh`) so that, when a test source file
`<name>.c` has a sibling `<name>.expected` file, the program's
stdout is captured and compared byte-for-byte against the expected
contents. Tests without a `.expected` file continue to be
validated by exit code alone.

The compiler itself is unchanged. The two existing puts tests
(`test_libc_puts_hello__0.c` and `test_libc_puts_two_calls__0.c`)
got `.expected` companion files (`hello\n` and `A\nB\n`
respectively), so the runtime stdout check is now exercised by
the suite.

## Plan

The plan followed the spec verbatim: capture stdout instead of
discarding it, do an optional byte-exact comparison against
`<name>.expected` when present, leave the exit-code check alone,
and back-fill the only known printing tests with `.expected`
files. No compiler change required.

## Changes Made

### Step 1: Test Runner — per-suite

- `test/valid/run_tests.sh`: redirect each binary's stdout to a
  per-test file under the work dir instead of `/dev/null`.
- After the existing exit-code check passes, look for
  `$SCRIPT_DIR/${name}.expected`; if present, compare via
  `diff -q` against the captured stdout. On mismatch, emit
  `FAIL  $name  (output mismatch)` followed by expected,
  actual, and unified diff.
- PASS line includes `, output matched` when an expected file
  was used, so it's visible in the suite log which tests
  exercised the new check.

### Step 2: Test Runner — single-test driver

- `test/valid/run_test.sh`: capture binary stdout to
  `${BASENAME}.stdout` before the exit-code branch.
- After the exit-code check, resolve the source dir from the
  argument path and look for `${BASENAME}.expected` next to the
  source. If present, byte-compare; FAIL with `diff` on
  mismatch.
- Cleanup line extended to remove the new stdout file.

### Step 3: Fixtures

- `test/valid/test_libc_puts_hello__0.expected` — `hello\n`
- `test/valid/test_libc_puts_two_calls__0.expected` — `A\nB\n`

### Step 4: Documentation

- `README.md`: `valid` row in the test-suites table now notes
  the optional `<name>.expected` stdout match.
- `docs/grammar.md`: no changes (no language change in this
  stage).

## Final Results

Build clean. 405 / 405 tests pass (252 valid + 49 invalid + 23
print-AST + 73 print-tokens + 8 print-asm). The two puts tests
now additionally pass the new stdout-vs-`.expected` comparison.

## Session Flow

1. Read the Stage 14-07-01 spec and supporting templates / notes.
2. Reviewed `test/valid/run_tests.sh`, `test/valid/run_test.sh`,
   the print_asm runner (already does file comparison), and the
   two puts tests added in Stage 14-07.
3. Produced a kickoff in `docs/kickoffs/` calling out the
   spec's grammar fragment and the byte-exact-comparison choice.
4. Modified the per-suite runner to capture stdout and add the
   optional `.expected` check.
5. Mirrored the change in the single-test driver.
6. Added the two `.expected` fixtures and verified their bytes
   with `xxd`.
7. Verified mismatch detection with a deliberately-wrong
   `.expected`, then restored.
8. Ran the full harness; recorded the milestone and transcript.

## Notes

- Comparison is byte-exact (`diff -q`), matching the existing
  `print_asm` suite. The spec did not specify; this is the
  conventional choice.
- Stdout capture moved off `/dev/null` and into a per-test file
  in the work dir; that file is reaped with the rest of the
  work dir at runner end.
- No compiler changes were necessary; the stage is purely a
  test-harness extension.
