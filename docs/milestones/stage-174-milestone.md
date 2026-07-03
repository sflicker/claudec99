# Milestone Summary

## Stage 174: Make --sysinclude Default; Add --testinclude - Complete

stage-174 flips the default include-path behavior in `bin/cc99`: on Linux x86_64, real system headers (`/usr/include`) are now used by default without requiring an explicit flag, while a new `--testinclude` option explicitly selects the project's stub headers. Platforms other than Linux x86_64 continue to default to stub headers (preserving prior behavior).

- Tokenizer: None.
- Parser: None.
- AST/Semantics: None.
- Codegen: None.
- Frontend: Modified `bin/cc99` to use tri-state `include_mode` instead of boolean `use_sysinclude`; added `--testinclude` option; implemented platform-based default (sysinclude on Linux x86_64, testinclude elsewhere); updated `--help` text.
- Tests: Two new `test/build_tool/` tests to verify include-path selection: `test_cc99_default_headers` (no flag → system headers) and `test_cc99_testinclude` (with flag → stub headers). Full suite: 2074 portable + 191 system-include + 2 optional-library + 5 build-tool (up from 3) = 2272/2272 pass.
- Docs: Updated `README.md` options table (`--sysinclude` now marked as Linux x86_64 default, new `--testinclude` row), added Stage 174 section to `docs/outlines/checklist.md`, updated version to `01740000` in `src/version.c`.
- Self-host: C0→C1→C2 bootstrap verified; all tests pass at every stage with zero source changes (bin/cc99 is shell script, not compiled during bootstrap).
- Notes: Only `test/build_tool/` fixtures invoke `bin/cc99` directly; all other test suites call the compiler with explicit `-I` flags and were unaffected. Incidental fix: removed stale CMake build cache (CMakeCache.txt) from `test/build_tool/test_cmake_hello/build/`, resolving a pre-existing test failure unrelated to this stage's changes.
