# Session Export — stage-180: macOS-Aware Test Infrastructure (Phase 0C)

**Date:** 2026-11-15
**Host:** macOS ARM64 (Darwin 25.5.0), Apple clang 21.0.0, NASM 3.02 (Homebrew), Rosetta 2
**Spec:** `docs/stages/ClaudeC99-spec-stage-180-macos-test-infrastructure.md`
**Commits:**

| Commit | What |
|--------|------|
| `6b86ee4` | spec and roadmap update (planning) |
| `309711c` | code |
| `af02927`, `291d8b5` | self-host checkpoints, committed by `build.sh` |
| `d865693` | self-compilation report |
| `b69860b` | docs |

---

## 1. Planning (before `/implement-stage`)

- **Outline check:** confirmed that `docs/outlines/arm64-macos-backend-roadmap.md` and `docs/outlines/checklist.md` were up to date for stage 179.
- **Investigation:** looked at the test runners, the `test/include` stub headers and the preprocessor's predefined macros.
- **Findings:**
  - macOS has no `timeout` command, and every runner uses it.
  - The runners hardcode `nasm -f elf64`, `gcc -no-pie` and `cc -no-pie`.
  - The stubs use glibc names (`stderr`, `__errno_location`).
  - ccompiler always predefines `__linux__` and never `__APPLE__`.
- **User decisions:**
  - Include the stub-header and predefined-macro fix in stage 180.
  - Defer `ARM64_TIER` to stage 181, because there is no ARM64 backend yet and roadmap tier 1 ("simple `statements/`") doesn't match the tree: `test/valid/statements/` holds only 6 switch tests.
- **Output:** wrote the stage 180 spec (Parts A–F) and added a Phase 0C scope note to the roadmap. Committed as `6b86ee4`.

## 2. `/implement-stage ClaudeC99-spec-stage-180-macos-test-infrastructure`

`STAGE_DISPLAY: stage-180`

### Pre-implementation checks

- `nasm -f macho64 -g -F dwarf` works, so `test_dwarf_debug` needs no skip.
- No runner and nothing in `build.sh` depends on `timeout`'s exit code 124. The perl fallback (SIGALRM, exit status 142) is safe.
- `--print-tokens` prints tokens after preprocessing, so it can be used to test macros under an explicit `--target` on any host.
- Stub constants:
  - Only the `stdio.h` streams and the `errno.h` accessor differ in a way tests exercise.
  - The `E*` values, `SEEK_*` and `_IO*BF` are identical on macOS.
  - `jmp_buf[32]` is at least as large as macOS needs.

### Spec issues raised; the user approved each resolution

1. The runner table omitted the four custom multi-file integration `run_test.sh` scripts. They were included.
2. The test design was open-ended. Two tests were added: a `test/valid` program for the host-default target, and a `--print-tokens` integration test that checks both targets explicitly.
3. *(found mid-implementation)* `test_pp_predef_linux_ifdef__42` and `test_pp_predef_unix_ifdef__42` assert `__linux__`/`__unix__` under the host default. That contradicts "no changes to existing tests" + "full macOS pass". Each test now returns 42 straight away when `__APPLE__` is defined; the Linux assertion is unchanged.
4. *(found after the first macOS suite run: 2160/2163)* On Mach-O, dylib data such as `__stdoutp` can't be reached RIP-relative (`ld: ... does not have address`). It also blocks the self-host, because the compiler's own sources use `stderr`. The user chose to fix it in stage 180 with GOT access, even though the spec excluded codegen changes.

### Kickoff

`docs/kickoffs/stage-180-kickoff.md` (`haiku-stage-artifact-writer`).

## 3. Implementation

A stage 179 Linux `.asm` baseline (1523 files) was captured before any change.

| File | Change |
|------|--------|
| `test/lib/platform.sh` (new) | `HOST_OS`/`HOST_ARCH`, `TARGET` (env override, host default), `COMPILER_TARGET_FLAG`, `NASM_FMT`, `LINK_CMD`, `run_timeout` (timeout → gtimeout → perl alarm); `TARGET=arm64-macos` errors |
| `test/valid/run_tests.sh`, `test/integration/run_tests.sh` | source `platform.sh`; target flag, nasm format, linker, `run_timeout` |
| `test/valid/run_test.sh`, `test/integration/run_test.sh` | same; `valid/run_test.sh`'s legacy `ld -e main` link replaced by `LINK_CMD` |
| `test/integration/test_{multi_file_success,designated_init_multifile,compound_literal_multifile,multi_file_error}/run_test.sh` | source `platform.sh`; target flag, nasm format, linker |
| `test/invalid`, `test/print_ast`, `test/print_tokens` runners | `run_timeout` only |
| `test/print_asm/run_tests.sh` | `run_timeout`; pinned to `--target=x86_64-linux` |
| `include/preprocessor.h`, `src/preprocessor.c` | `preprocess_set_target(Target)`; `x86_64-macos` predefines `__APPLE__`/`__MACH__` instead of `__linux__`/`__unix__` |
| `src/compiler.c` | calls `preprocess_set_target(target)` after target resolution |
| `test/include/stdio.h`, `test/include/errno.h` | `__APPLE__` branches: `__stdinp`/`__stdoutp`/`__stderrp`, `__error()` |
| `src/codegen.c` | Mach-O GOT access for `extern` data (details below) |
| `build.sh` | inline `run_timeout` replaces both `timeout` calls |
| `src/version.c` | `VERSION_STAGE "01800000"` |
| Tests | new `test/valid/preprocessor/test_target_platform_macros__42.c`; new `test/integration/test_target_predefined_macros/` (`macros.c` + `run_test.sh`); 2 existing predef tests get the `__APPLE__` pass-through |

The `src/codegen.c` GOT change adds three helpers:

- `global_via_got`: true for an `extern` data object on the Mach-O target.
- `global_rel_or_got`: used by the four global load/store helpers. It emits `mov r11, [rel _x wrt ..gotpcrel]` and addresses `[r11]`.
- `emit_global_addr`: used by the nine address sites. It emits `mov rax, [rel _x wrt ..gotpcrel]` instead of `lea`.

Notes from the session:

- One accidental regex replacement turned the `else` branch of `emit_global_addr` into a recursive self-call. It was caught on review and restored before the build.
- The first version of the integration test's `awk` read field `$4`. That broke on the padded token index (`[1 ]`), so it now finds the `TOKEN::` label instead.

## 4. Verification

| Check | Result |
|-------|--------|
| Macro output per target (`--print-tokens`) | linux: `__linux__ __unix__`; macos and host default: `__APPLE__ __MACH__` |
| Linux regression | Linux-target `.asm` for all test sources byte-identical to stage 179; only the `__TIME__` test and the new tests differ; same set of files that fail to compile |
| macOS suite, first run | 2160/2163 (3 link errors: `___stdoutp` / `___stderrp` "does not have address") |
| macOS suite, after GOT fix | **2163/2163** — 249 unit, 1287 valid, 261 invalid, 195 integration, 50 print-AST, 100 print-tokens, 21 print-asm (stage 179 baseline on this host: 250 passed / 1911 failed) |
| Quick bootstrap (`--mode=bootstrap`) | succeeded; C1 is x86_64 Mach-O and auto-detects `x86_64-macos` |
| Self-host (`--mode=self-host`, macOS) | **C0, C1 and C2 each passed 2163/2163** (versions below) |

Self-host versions:

| Step | Version | Built by |
|------|---------|----------|
| C0 | `00.03.01800000.01306` | AppleClang |
| C1 | `00.03.01800000.01307` | C0 |
| C2 | `00.03.01800000.01308` | C1 |

Linker warnings (not errors) when linking C1/C2 on macOS:

- `pointer not aligned` (about 70, in `peephole.o`/`codegen.o`): pointer tables in data sections are not 8-byte aligned.
- `no platform load command` (one per object file): NASM's Mach-O output has no `LC_BUILD_VERSION`.

## 5. Documentation

- `docs/self-compilation-report.md`: stage 180 macOS self-host entry, with issues fixed, warnings and the C0/C1/C2 table (`d865693`).
- `docs/outlines/arm64-macos-backend-roadmap.md`: Phase 0C checked off; added a note about the GOT addition; "Next up" is now stage 181.
- Written by `haiku-stage-artifact-writer`:
  - `docs/milestones/stage-180-milestone.md`
  - `docs/sessions/stage-180-transcript.md`
  - `docs/outlines/checklist.md`: Stage 180 section; no TODO item completed.
  - `README.md`:
    - new "Through Stage 180" entry
    - self-host row notes AppleClang C0 on macOS and Rosetta for C1/C2
    - one-line note about `TARGET` and `platform.sh`
- `docs/grammar.md`: unchanged, since the grammar did not change.

## 6. Follow-ups

- Run the Linux x86-64 self-host for stage 180.
- macOS linker warnings: data pointer alignment, and the missing platform load command.
- `bin/cc99` macOS support.
- Stage 181: ARM64 codegen skeleton plus `ARM64_TIER` gating, with the tier-1 test set defined then.
