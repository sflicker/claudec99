# Stage 180 Kickoff: macOS-Aware Test Infrastructure (Phase 0C)

## Derived Values

| Item | Value |
|------|-------|
| STAGE_LABEL | stage-180 |
| STAGE_DISPLAY | stage-180 |
| Spec | `docs/stages/ClaudeC99-spec-stage-180-macos-test-infrastructure.md` |
| New helper | `test/lib/platform.sh` (HOST_OS, HOST_ARCH, TARGET, COMPILER_TARGET_FLAG, NASM_FMT, LINK_CMD, run_timeout) |
| New preprocessor API | `void preprocess_set_target(Target)` in `include/preprocessor.h` |
| New tests | `test/valid/preprocessor/test_target_platform_macros__42.c`, `test/integration/test_target_predefined_macros/` |
| VERSION_STAGE | `01800000` (stage 179 set `01790000`) |
| Expected output | Linux `.asm` byte-identical to stage 179; macOS suite and C0→C1→C2 self-host pass under Rosetta 2 |

## 1. Spec Summary

Stage 180 is Phase 0C of the ARM64/macOS roadmap. It makes the full portable test suite and the C0→C1→C2 self-host run on a macOS ARM64 host, using the Stage 179 `x86_64-macos` target under Rosetta 2. The Linux x86-64 path is unchanged.

On macOS, `./test/run_all_tests.sh` currently reports 250 passed and 1911 failed. Three host problems cause this:

1. **No `timeout` command on macOS.** Every runner and `build.sh` calls `timeout`, so each test is reported as a compiler error.
2. **Runners hardcode ELF/Linux tools:** `nasm -f elf64`, `gcc -no-pie`, `cc -no-pie`.
3. **Stub headers are glibc-only, and ccompiler never predefines `__APPLE__`.** `test/include/stdio.h` uses `stdin`/`stdout`/`stderr` (macOS exports `__stdinp`/`__stdoutp`/`__stderrp`). `test/include/errno.h` uses `__errno_location()` (macOS uses `__error()`). Four `test/valid` programs fail to link on macOS, and a macOS self-host fails because the compiler sources use `stderr`. The same gap means a macOS-built self-hosted ccompiler auto-detects the Linux target.

The spec adds no tokenizer, parser, AST, grammar, or codegen changes. The only compiler-source change is target-dependent predefined macros in the preprocessor.

### Planned changes

- **`test/lib/platform.sh` (new):** `TARGET` defaults to `x86_64-macos` on Darwin and `x86_64-linux` otherwise, and can be overridden from the environment. Sets `NASM_FMT` (`elf64` / `macho64`) and `LINK_CMD` (`gcc -no-pie` / `clang -arch x86_64`). `TARGET=arm64-macos` errors until Stage 181. `run_timeout` tries `timeout`, then `gtimeout`, then a perl `alarm` fallback.
- **Runners:**
  - `test/valid` and `test/integration` (`run_tests.sh` and `run_test.sh`): timeout, target flag, nasm format, linker.
  - The three custom integration scripts `test/integration/{test_multi_file_success,test_designated_init_multifile,test_compound_literal_multifile}/run_test.sh`: same changes. `test_multi_file_error/run_test.sh` is compile-only and changes only if needed.
  - `test/invalid`, `test/print_ast`, `test/print_tokens`: timeout only.
  - `test/print_asm`: timeout, and always `--target=x86_64-linux`, because its `.expected` files are Linux assembly text and the suite never assembles.
  - `test/unit`: no change.
- **`include/preprocessor.h` / `src/preprocessor.c`:** add `preprocess_set_target(Target)`. Default is `TARGET_X86_64_LINUX`. The existing `preprocess*` signatures stay the same, because unit tests call them directly.
  - `x86_64-linux`: unchanged.
  - `x86_64-macos`: defines `__APPLE__` and `__MACH__` as `1`, and does not define `__linux__` or `__unix__`.
  - `__x86_64__`, `__LP64__`, `_LP64`, `__CHAR_BIT__`, `__SIZEOF_*__` are unchanged on both.
- **`src/compiler.c`:** call `preprocess_set_target(target)` after target resolution and before the per-file loop. This also makes the `#if defined(__APPLE__)` auto-detect select `x86_64-macos` in a self-hosted C1 built on macOS.
- **`test/include/stdio.h`:** `__APPLE__` branch maps `stdin`/`stdout`/`stderr` to `__stdinp`/`__stdoutp`/`__stderrp`.
- **`test/include/errno.h`:** `__APPLE__` branch uses `int *__error(void)`.
- **`build.sh`:** inline `run_timeout` (same fallback chain) replaces its two `timeout` calls. It does not source `platform.sh`. No other change.
- **`src/version.c`:** `VERSION_STAGE "01800000"`.

## 2. Required Tokenizer Changes

None.

## 3. Required Parser Changes

None. `docs/grammar.md` stays as is.

## 4. Required AST Changes

None.

## 5. Required Code Generation Changes

None in `src/codegen.c`. The Stage 179 Mach-O symbol handling is reused unchanged. The only compiler-source change is the preprocessor predefined-macro block (`src/preprocessor.c`, "Target/ABI predefined macros"), plus the `preprocess_set_target` call in `src/compiler.c`.

## 6. Test Plan

### New tests

- **(a) `test/valid/preprocessor/test_target_platform_macros__42.c`:** runs under the host-default target. Returns 42 if and only if exactly one of `__linux__` / `__APPLE__` is defined, `__APPLE__` ⇔ `__MACH__`, and `__linux__` ⇔ `__unix__`. The exit code is 42 on both hosts.
- **(b) `test/integration/test_target_predefined_macros/`:** custom script. Runs `ccompiler --target=x86_64-linux --print-tokens` and `ccompiler --target=x86_64-macos --print-tokens` on a source with `#if defined(...)` marker identifiers, and checks that the correct markers appear under each target. Host-independent, because `--print-tokens` output is post-preprocessing.

No existing tests change.

### Verification

1. **Linux x86_64 regression:** `./test/run_all_tests.sh` totals match Stage 179 (2161 portable, plus the Linux-gated counts, plus the new tests). Generated `.asm` for every test source is byte-identical to Stage 179 under the Linux target, except the `__TIME__` test. `./build.sh --mode=self-host` passes C0→C1→C2.
2. **macOS ARM64 (Rosetta 2):** `./test/run_all_tests.sh` passes the full portable suite. Any remaining failure is recorded in the milestone with root cause and a follow-up, not skipped. `./build.sh --mode=self-host` completes C0→C1→C2 with tests passing at each step.
3. **Self-compilation report:** `docs/self-compilation-report.md` records C0/C1/C2 results for both hosts. If the Linux run cannot be done from the implementing host, record it as pending.

## Findings and Spec Issues (pending user confirmation)

These were found while preparing this kickoff. The proposed resolutions have not been confirmed.

1. **Spec Part B omits the three custom integration scripts** that hardcode `nasm -f elf64` and `cc -no-pie`. *Proposed:* include them, sourcing `platform.sh`.
2. **Spec test section is open-ended.** *Proposed:* tests (a) and (b) above.
3. **`nasm -f macho64 -g -F dwarf` is accepted** (NASM 3.02, verified). *Resolution:* `test_dwarf_debug` needs no skip.
4. **No runner or `build.sh` depends on the exit code 124 from `timeout`.** The perl fallback dies by SIGALRM and bash reports 142. *Resolution:* the fallback is safe. Note this in a comment in `platform.sh`.
5. **Stub constants checked against macOS.** Only the stdio stream names and the errno accessor differ in ways the tests exercise. `E*` errno values, `SEEK_*`, and `_IO*BF` are identical on macOS. `jmp_buf[32]` (256 bytes) is at least the macOS x86_64 `jmp_buf` size. `BUFSIZ` and `FILENAME_MAX` differ but are not ABI-relevant, so they stay unchanged and are listed in the milestone.
6. **macOS C0 is built with AppleClang** (acknowledged in the spec). The libc availability of ctype, time, and setjmp symbols on macOS is unverified until the macOS self-host runs.

## Implementation Plan

Order:

1. `test/lib/platform.sh`
2. Runners (valid, integration, invalid, print_ast, print_tokens, print_asm, custom integration scripts)
3. `include/preprocessor.h`, `src/preprocessor.c`, `src/compiler.c`
4. Stub headers (`test/include/stdio.h`, `test/include/errno.h`)
5. `build.sh` inline `run_timeout`
6. `src/version.c`
7. New tests (a) and (b)
8. Linux `.asm` regression and macOS full suite
9. macOS self-host (C0→C1→C2), then Linux self-host if available
10. Docs (milestone, transcript, self-compilation report, README, roadmap)
11. Commit (only when the user asks)

Pause after each step for review.

## Notes

- `ARM64_TIER` is deferred to Stage 181. Stage 180 only reserves `TARGET=arm64-macos`, which errors in `platform.sh`.
- Parts C and D (predefined macros and stub headers) are added to Phase 0C. The roadmap did not list them, but neither the macOS suite nor the macOS self-host can pass without them.
- Out of scope: `bin/cc99` macOS support, running Linux-gated suites on macOS, ARM64 codegen, and any language or codegen change beyond the Part C predefined macros.
- This kickoff covers planning only. No compiler source, tests, runners, or `build.sh` changes are made by this artifact.
