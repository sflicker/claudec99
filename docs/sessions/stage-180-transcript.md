# stage-180 Transcript: macOS-Aware Test Infrastructure

## Summary

Stage 180 makes the test harness and the preprocessor target-aware so the full portable suite runs on an Apple Silicon host (macOS ARM64, executing x86-64 Mach-O under Rosetta 2). Before this stage, the runners were ELF-only and assumed `timeout`, `ld -e main`, and Linux predefined macros; on macOS only 250 of 2161 portable tests passed. The stage adds a shared runner configuration, `test/lib/platform.sh`, which every runner sources. It selects the target (`TARGET` env override, default `x86_64-macos` on Darwin and `x86_64-linux` elsewhere), the NASM output format, the link command, and a timeout wrapper that falls back from `timeout` to `gtimeout` to a perl alarm.

The preprocessor gains `preprocess_set_target()`. Under `x86_64-macos` it predefines `__APPLE__` and `__MACH__` in place of `__linux__` and `__unix__`, so a self-hosted macOS binary defaults to the correct target. On Mach-O, codegen now reaches `extern` data through the GOT, because dylib data such as `__stdoutp` cannot be addressed RIP-relative. Linux output is byte-identical to stage 179.

Constraints: no tokenizer, parser, AST, or grammar changes. `TARGET=arm64-macos` is rejected by the runners until Stage 181, which owns `ARM64_TIER`. System-include, optional-library, and build-tool suites are Linux-only and were not run on this host. The Linux x86-64 self-host is pending.

## Plan

The kickoff (`docs/kickoffs/stage-180-kickoff.md`) listed open items that were resolved during implementation. The spec's runner table omitted the custom multi-file integration scripts, so they were included. The spec said "no changes to existing tests," but two Linux-predefine tests asserted `__linux__`/`__unix__` under the host default, so each gained an `#if defined(__APPLE__)` pass-through that returns 42 while the Linux assertion stays unchanged. The GOT codegen change was added beyond the spec to reach a full macOS pass and a successful self-host. The `nasm -f macho64 -g -F dwarf` path was verified with NASM 3.02, so `test_dwarf_debug` needs no skip.

Implementation order: `test/lib/platform.sh`, runner migration, `build.sh` timeout, preprocessor target hook, stdio/errno stub branches, GOT codegen helpers, new tests, verification (macOS portable suite, Linux `.asm` regression, self-host), docs.

## Changes Made

### Step 1: Shared Test Platform Config

- Added `test/lib/platform.sh` with `HOST_OS`/`HOST_ARCH` detection and `TARGET` env override (default `x86_64-macos` on Darwin, else `x86_64-linux`).
- `platform.sh` sets `COMPILER_TARGET_FLAG`, `NASM_FMT` (`macho64` or `elf64`), and `LINK_CMD` (`clang -arch x86_64` or `gcc -no-pie`).
- `platform.sh` defines `run_timeout` with fallback order `timeout`, `gtimeout`, then perl alarm.
- `platform.sh` errors when `TARGET=arm64-macos` is set (Stage 181).

### Step 2: Test Runner Migration

- `test/valid/run_tests.sh` and `test/valid/run_test.sh`: source `platform.sh`, use `run_timeout`, pass `COMPILER_TARGET_FLAG`, use `NASM_FMT`, and replace the legacy `ld -e main` link with `LINK_CMD`.
- `test/integration/run_tests.sh` and `test/integration/run_test.sh`: same migration.
- Custom integration scripts `test_multi_file_success`, `test_designated_init_multifile`, `test_compound_literal_multifile`, and `test_multi_file_error`: source `platform.sh` and use `LINK_CMD` and `run_timeout`.
- `test/invalid/run_tests.sh`: source `platform.sh` and use `run_timeout`.
- `test/print_ast/run_tests.sh` and `test/print_tokens/run_tests.sh`: source `platform.sh` and use `run_timeout`. `print_tokens` is timeout-only.
- `test/print_asm/run_tests.sh`: source `platform.sh`, use `run_timeout`, and pin `--target=x86_64-linux` because its `.expected` files are Linux asm text.

### Step 3: Preprocessor Target Macros

- `include/preprocessor.h`: declared `void preprocess_set_target(Target)`.
- `src/preprocessor.c`: `x86_64-macos` predefines `__APPLE__` and `__MACH__` and omits `__linux__` and `__unix__`. Other targets keep the Linux set.
- `src/compiler.c`: calls `preprocess_set_target` after target resolution.

### Step 4: Codegen GOT Access on Mach-O

- `src/codegen.c`: added `global_via_got`, `global_rel_or_got`, and `emit_global_addr`. On Mach-O, extern data loads its address with `mov r11, [rel _x wrt ..gotpcrel]` and the operand becomes `[r11]`. `emit_global_addr` uses `mov rax, [rel _x wrt ..gotpcrel]` in place of `lea`.
- Linux codegen paths are unchanged. The Linux `.asm` output is byte-identical.

### Step 5: Build Script

- `build.sh`: the two `timeout` invocations are replaced with an inline `run_timeout`.

### Step 6: Stub Headers

- `test/include/stdio.h`: `__APPLE__` branch maps `stdin`/`stdout`/`stderr` to `__stdinp`/`__stdoutp`/`__stderrp`.
- `test/include/errno.h`: `__APPLE__` branch uses `__error()`.

### Step 7: Tests

- New `test/valid/preprocessor/test_target_platform_macros__42.c`: under the host-default target, exactly one OS family is predefined, the paired macros are consistent, and `__x86_64__` is present.
- New `test/integration/test_target_predefined_macros/` (`macros.c` and `run_test.sh`): runs `--print-tokens` under both explicit `--target` values and checks the surviving identifiers. The test is host-independent.
- `test_pp_predef_linux_ifdef__42` and `test_pp_predef_unix_ifdef__42`: added an `#if defined(__APPLE__)` pass-through returning 42. The Linux assertions are unchanged.

### Step 8: Version and Docs

- `src/version.c`: `VERSION_STAGE` set to `"01800000"`.
- `docs/milestones/stage-180-milestone.md`, `docs/sessions/stage-180-transcript.md`, and the Stage 180 section in `docs/outlines/checklist.md` added.
- `README.md`: Through Stage 180 entry, self-host notes for macOS ARM64, and a `TARGET`/`platform.sh` pointer in the Tests section.
- `docs/grammar.md` not changed (no grammar change).

## Final Results

- **Build**: Successful. `bin/ccompiler` rebuilt. `VERSION_STAGE` is `01800000`.
- **Tests (macOS ARM64, Rosetta 2)**: Portable 2163 passed, 0 failed, 2163 total (249 unit, 1287 valid, 261 invalid, 195 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Before this stage on this host: 250 passed, 1911 failed.
- **Self-host (macOS ARM64)**: C0 `00.03.01800000.01306` (AppleClang, arm64), C1 `00.03.01800000.01307` (built by C0, x86-64 Mach-O), C2 `00.03.01800000.01308` (built by C1). Each stage passes 2163/2163 portable.
- **Regression**: Linux-target `.asm` for all test sources is byte-identical to stage 179, except the `__TIME__` test and the new tests. Portable totals rose from 2161 to 2163 (+1 valid, +1 integration). README Linux system-include, optional-library, and build-tool counts are unchanged and were not re-run.
- **Not run**: Linux x86-64 self-host (not runnable from this host); system-include, optional-library, and build-tool suites (Linux-only).

## Session Flow

1. Read the stage-180 spec, the kickoff, and the stage-179 milestone and transcript style references.
2. Resolved kickoff open items: runner table extension, the two Linux-predefine test pass-throughs, and the GOT codegen addition.
3. Added `test/lib/platform.sh` and migrated the runners and custom integration scripts to it.
4. Added the preprocessor target hook, stub header `__APPLE__` branches, and Mach-O GOT access in codegen.
5. Added the two new tests and updated the two preprocessor predefine tests.
6. Built and ran the macOS ARM64 portable suite to 2163/2163, and ran the Linux `.asm` byte-identity regression.
7. Ran the macOS self-host cycle (C0 → C1 → C2), each stage 2163/2163, with checkpoint commits `af02927` and `291d8b5`.
8. Wrote the milestone, transcript, checklist, and README artifacts. The self-host report (`d865693`) was produced separately.

## Notes

- The `ld` warnings on macOS link (`pointer not aligned` in `peephole.o`/`codegen.o` data tables, and `no platform load command` for NASM Mach-O objects without `LC_BUILD_VERSION`) are harmless and recorded as follow-ups.
- Follow-ups for later stages: Linux x86-64 self-host verification, `bin/cc99` macOS support, and `ARM64_TIER` plus the ARM64 skeleton in Stage 181.

## Commit

- Implementation: `309711c` (stage 180: macOS-aware test infrastructure, Phase 0C).
- Self-host checkpoints: `af02927` (C0), `291d8b5` (C1).
- Self-host report: `d865693` (docs: stage 180 self-compilation report, macOS ARM64 self-host).
