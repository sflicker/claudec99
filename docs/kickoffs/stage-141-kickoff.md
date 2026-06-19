# Stage 141 Kickoff — System Includes

## Spec Summary

Stage 141 adds a `--sysinclude` flag to the `bin/cc99` wrapper that switches
from the project's `test/include` stub headers to the real Linux system include
paths (`/usr/lib/gcc/x86_64-linux-gnu/13/include`, `/usr/local/include`,
`/usr/include/x86_64-linux-gnu`, `/usr/include`).  The flag is only available
on Linux x86\_64 (errors on other platforms).

The test suite runner (`test/run_all_tests.sh`) is extended so that, when
executing on Linux x86\_64, it also runs
`test/integration/run_tests_sysinclude.sh` and reports results in a separate
"System include" section, distinct from the portable suite aggregate.

## Required Changes

| Component | Change |
|-----------|--------|
| `bin/cc99` | Add `--sysinclude` option; platform-guard on Linux x86_64 |
| `test/run_all_tests.sh` | Detect Linux x86_64; run sysinclude suite; split totals |
| `src/version.c` | Bump stage to `14100000` |

**No tokenizer, parser, AST, or codegen changes.**

## Ambiguities / Decisions

None — spec is clear and self-contained.

## Implementation Order

1. `bin/cc99` — add `--sysinclude` flag
2. `test/run_all_tests.sh` — add system include section
3. `src/version.c` — bump stage
4. Manual smoke-test with `--sysinclude` on a simple program
5. Run full test suite to confirm no regressions
6. Commit
