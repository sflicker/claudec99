# Stage 174 Kickoff: Make --sysinclude Default; Add --testinclude Option

## 1. Summary of the spec

Currently, `cc99` defaults to using the project's stub headers in `test/include/` (for platform independence) unless the user explicitly passes `--sysinclude` (which switches to real system headers under `/usr/include`, gated to Linux x86_64 only). This stage flips that default: on Linux x86_64, `--sysinclude` behavior becomes the default with no flag needed. A new `--testinclude` flag is added to explicitly request the old stub-header behavior. On any platform that isn't Linux x86_64, the default remains stub headers (equivalent to today's behavior), since real system headers are not available/supported there.

## 2. Required tokenizer changes

None.

## 3. Required parser changes

None.

## 4. Required AST changes

None.

## 5. Required code generation changes

None.

## 6. Frontend shell script changes

### bin/cc99 modifications

- Replace the boolean `use_sysinclude` variable with a tri-state `include_mode` (values: `""`, `"sysinclude"`, or `"testinclude"`).
- Add a `--testinclude` option case that sets `include_mode="testinclude"`.
- After argument parsing, if `include_mode` is still unset (empty string), default it based on platform: set to `"sysinclude"` on Linux x86_64, otherwise `"testinclude"`. Reuse the existing `uname -s`/`uname -m` check that gates the explicit `--sysinclude` flag for consistency.
- Update the compile loop's active-include-flags selection to branch on `include_mode` instead of the old boolean flag.
- Update `--help` text to document `--testinclude` and note that `--sysinclude` is now the default on Linux x86_64.

### README.md documentation

- Update the `bin/cc99` options table: add a new `--testinclude` row describing the option, and update the existing `--sysinclude` row to note it is now the default on Linux x86_64.

### src/version.c

- Bump `VERSION_STAGE` to `01740000`.

### docs/outlines/checklist.md

- Add a new `## Stage 174 - Make --sysinclude the Default; Add --testinclude` section.

## 7. Test plan

### Existing tests unchanged

Investigation confirmed that only `test/build_tool/` tests (`test_make_hello`, `test_make_cflags`) actually invoke `bin/cc99` directly. All other test suites call `build/ccompiler` directly with explicit `-I` flags, bypassing cc99's default logic entirely, so they are unaffected by this change.

### Two new tests under test/build_tool/

- **test_cc99_default_headers:** Compiles with no explicit include flag, expects real system headers (Linux x86_64 only) to be selected. Uses the stub header's unique include-guard macro `CLAUDEC99_STDIO_H` (defined only in `test/include/stdio.h`, never in real system headers) to distinguish which header set was actually used.

- **test_cc99_testinclude:** Compiles with `--testinclude` flag, expects the project's stub headers to be selected. Also uses the `CLAUDEC99_STDIO_H` macro to confirm the correct header set was found.

## 8. Design decisions

- **Platform gate consistency:** The default-selection logic reuses the existing Linux x86_64 check (not just "Linux"), for consistency with the existing `--sysinclude` gate and because the hardcoded `/usr/include/...` system paths are x86_64-specific.

- **Flag precedence:** If a user passes both `--sysinclude` and `--testinclude`, last flag wins (standard CLI convention); no error is thrown.

## 9. Files to change

- `bin/cc99` — tri-state include mode, `--testinclude` option, platform-based default, help text
- `README.md` — options table update
- `test/build_tool/test_cc99_default_headers` — new test (create)
- `test/build_tool/test_cc99_testinclude` — new test (create)
- `src/version.c` — stage bump
- `docs/outlines/checklist.md` — new stage section

## 10. Status

Tests run/result: not yet run (kickoff is pre-implementation).  
Files changed so far: none (kickoff is pre-implementation).
