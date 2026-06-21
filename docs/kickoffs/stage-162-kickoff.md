# Stage 162 Kickoff — Add zlib Integration Test

## Spec Summary

Stage 162 adds an optional-library integration test for the zlib compression
library to `test/integration_sysinclude/`. The test program is taken directly
from the stage 158 spec: compress "Hello From ClaudeC99" with zlib's
`compress()` function and print the original and compressed byte counts.
A `.require` companion file gates the test on `pkg-config --exists zlib` so
it is auto-skipped when zlib is not installed. README gets a zlib row in the
optional-library table.

## Stage Values

- STAGE_LABEL: stage-162
- STAGE_DISPLAY: Stage 162
- VERSION_STAGE: 01620000

## What Changes vs Stage 161

Stage 161 was a compiler fix (void * comparison). Stage 162 is purely additive:
no compiler source changes, only new test files, a version bump, and doc updates.

## Ambiguities / Decisions

- **No `.expected` file**: the compressed byte count depends on the zlib
  version installed; checking exit status 0 is sufficient.
- **No `.cflags` file**: the runner's `DEFAULT_IFLAGS` already includes
  `/usr/include`, so zlib.h is found without extra flags.
- **Require check**: `pkg-config --exists zlib` chosen to parallel the SDL2
  test pattern (`sdl2-config --version`).

## Implementation Plan

1. Create `test/integration_sysinclude/test_zlib_compress/` with 4 files
2. Bump `src/version.c` to `01620000`
3. Update `README.md` (optional-library table + stage blurb + test totals)
4. Update `docs/outlines/checklist.md` (new Stage 162 section)
5. Run test suite — verify 2 optional-library tests pass
6. Commit; run `./build.sh --mode=self-host`
7. Update `docs/self-compilation-report.md`
8. Generate milestone and transcript artifacts
