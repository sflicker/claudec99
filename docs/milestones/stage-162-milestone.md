# Stage 162 Milestone — Add zlib Integration Test

## Summary

Stage 162 adds `test_zlib_compress` to the optional-library integration suite
(`test/integration_sysinclude/`), bringing the optional-library test count
from 1 to 2.

## What Was Done

### New test: test_zlib_compress

Four companion files added to `test/integration_sysinclude/test_zlib_compress/`:

- **test_zlib_compress.c** — compresses the string "Hello From ClaudeC99"
  using zlib's `compress()` function, prints original (21) and compressed byte
  counts, returns 0 on success.
- **test_zlib_compress.require** — `pkg-config --exists zlib`; test is
  auto-skipped when zlib is not installed.
- **test_zlib_compress.libs** — `-lz`
- **test_zlib_compress.status** — `0`

No `.cflags` file is needed: the runner's `DEFAULT_IFLAGS` already include
`/usr/include` where `zlib.h` lives. No `.expected` file: compressed size
varies by zlib version; exit status 0 is the correctness criterion.

### Version bump

`src/version.c` `VERSION_STAGE` updated to `"01620000"`.

### Documentation

- `README.md`: zlib row added to optional-library table; stage blurb and
  test totals updated (2 optional-library tests).
- `docs/outlines/checklist.md`: new Stage 162 section added.
- `docs/self-compilation-report.md`: Stage 162 result table added.

## Test Results

| Suite | Result |
|-------|--------|
| Portable | 2065 / 2065 |
| System-include | 182 / 182 |
| Optional-library | 2 / 2 (0 skipped) |

## Self-Host

C0 → C1 → C2 verified. No compiler source changes were needed during bootstrap.

| Step | Version |
|------|---------|
| C0 | 00.03.01620000.01200 |
| C1 | 00.03.01620000.01201 |
| C2 | 00.03.01620000.01202 |
