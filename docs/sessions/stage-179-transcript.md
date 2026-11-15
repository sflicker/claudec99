# stage-179 Transcript: x86-64 Mach-O Cross-Compile Target

## Summary

Stage 179 adds an `x86_64-macos` code generation target (Phase 0B of the ARM64/macOS roadmap). The compiler, running on an Apple Silicon host, now emits x86-64 NASM that `nasm -f macho64` assembles into Mach-O64 objects, which `clang -arch x86_64` links and which run under Rosetta 2. The backend instruction selection, register model, and section layout are unchanged from the Linux backend. Only two output details differ: C-visible symbol names receive a `_` prefix, and the ELF-only `section .note.GNU-stack` footer is omitted. Internal compiler labels (`Lstr*`, `Lfc*`, `Lfp_*`, `Lstatic_*`, `.L*`) are never prefixed.

The target is selected with `--target=x86_64-linux|x86_64-macos|arm64-macos`. When the flag is omitted, the driver auto-detects the host with `#if defined(__APPLE__)`. `build.sh` always passes `--target` explicitly, so bootstrap does not depend on the auto-detect. `--target=arm64-macos` is accepted by the parser but exits with "not yet implemented"; ARM64 codegen is Phase 1+. Unknown target names are an error.

Constraints: no tokenizer, parser, AST, or grammar changes, and no new tests (the spec lists none). The macOS test runners are ELF-only until Phase 0C, and this host has no `timeout` command, so the self-host cycle was not run here. Verification relied on a byte-identical Linux `.asm` regression and on macOS end-to-end runs.

## Plan

The kickoff (`docs/kickoffs/stage-179-kickoff.md`) recorded seven open items. Items 1 to 3 were resolved during implementation: the spec table was extended to cover shared helpers, `init_label` gets an explicit internal-label flag, and the `__APPLE__` auto-detect gap was accepted because `build.sh` passes `--target`. Items 4 to 7 (no spec tests, self-host deferred, VERSION_STAGE, drifted line numbers) were handled by the verification plan below and by re-deriving line numbers from the code.

Implementation order: `include/target.h`, `src/compiler.c` flag parsing and auto-detect, `include/codegen.h` fields, `src/codegen.c` prefix sites, `build.sh` platform selection, `src/version.c` bump, verification, docs.

## Changes Made

### Step 1: Target Plumbing

- Added `include/target.h` with `Target` enum: `TARGET_X86_64_LINUX`, `TARGET_X86_64_MACOS`, `TARGET_ARM64_MACOS`, guarded by `CCOMPILER_TARGET_H`.
- `src/compiler.c`: included `target.h`; added `Target target` (default `TARGET_X86_64_LINUX`) and `target_set` flag.
- `src/compiler.c`: parses `--target=` after the flag loop. `x86_64-linux` and `x86_64-macos` map to their enum values; `arm64-macos` maps to `TARGET_ARM64_MACOS`; any other value prints `error: unknown target '...'` and exits 1.
- `src/compiler.c`: when `--target` is omitted, sets `TARGET_X86_64_MACOS` under `#if defined(__APPLE__)` and `TARGET_X86_64_LINUX` otherwise.
- `src/compiler.c`: `TARGET_ARM64_MACOS` prints `error: target 'arm64-macos' is not yet implemented` and exits 1.
- `src/compiler.c`: `target` is threaded through `compile_one_file()`.
- `src/compiler.c`: `--help` lists `--target=<target>` with the supported values.

### Step 2: Codegen State

- `include/codegen.h`: added `int target_macho` to `CodeGen`.
- `include/codegen.h`: added `int init_label_is_internal` to `GlobalVar` (resolves kickoff open item 2).
- `src/compiler.c`: sets `cg.target_macho = (target == TARGET_X86_64_MACOS);` after `codegen_init(&cg, out)`.
- `src/codegen.c`: `codegen_init` zeroes `target_macho`.

### Step 3: Codegen Symbol Prefixing

- `src/codegen.c`: added `static const char *sp(const CodeGen *cg)` returning `"_"` on Mach-O and `""` otherwise.
- `src/codegen.c`: `global` and `extern` directives (function, global, and extern names) use `sp(cg)`.
- `src/codegen.c`: function label definitions and global `db`/`dd`/`dq`/`resb` label definitions use `sp(cg)`, including FP-literal global label definitions that the spec table omitted.
- `src/codegen.c`: `call` instructions (three sites) use `sp(cg)`.
- `src/codegen.c`: `[rel name]` references (`lea` and `mov` sites) use `sp(cg)`.
- `src/codegen.c`: `dq` address-constant initializers for file-scope statics (`name`, `Lstr` pointers, `arr + N`) use `sp(cg)` on the C-name side only.
- `src/codegen.c`: `emit_load_global`, `emit_store_global`, `emit_load_fp_global`, and `emit_store_fp_global` gained an explicit `pfx` parameter. Callers pass `sp(cg)` for C globals and `""` for `Lstatic_` labels (resolves kickoff open item 1).
- `src/codegen.c`: global `init_label` emission uses `sp(cg)` only when `init_label_is_internal` is 0; the string-literal branch sets the flag to 1 so `Lstr*` labels stay unprefixed (resolves kickoff open item 2).
- `src/codegen.c`: `codegen_emit_local_statics` `dq` RHS names use `sp(cg)`; the `Lstatic_` left-hand labels do not.
- `src/codegen.c`: the `.note.GNU-stack` footer is wrapped in `if (!cg->target_macho)`.

### Step 4: Build Script

- `build.sh`: added `HOST_OS=$(uname -s)` and `HOST_ARCH=$(uname -m)`.
- `build.sh` `do_bootstrap_build()`: selects `NASM_FMT` (`macho64` or `elf64`), `LINK_CMD` (`clang -arch x86_64` or `gcc -no-pie`), and `COMPILER_TARGET` (`--target=x86_64-macos` or `--target=x86_64-linux`) by host.
- `build.sh`: passes `$COMPILER_TARGET` to the bootstrap compiler invocation, uses `$NASM_FMT` for `nasm -f`, and uses `${LINK_CMD[@]}` for the final link.

### Step 5: Version

- `src/version.c`: `VERSION_STAGE` set to `"01790000"` (stage 178 left it at `01770000`; the skipped value is noted in Notes).

### Step 6: Verification

- Linux regression: compiled every `test/**/*.c` (`test/valid`, `test/integration`, `test/print_asm`) and the unchanged `src/*.c` with `--target=x86_64-linux` on both the pre-change and post-change compilers. The `.asm` outputs are byte-identical across 1523 files; the only difference is the `__TIME__` test, which is expected.
- macOS end-to-end: compiled `test/valid` programs with auto-detected target, assembled with `nasm -f macho64`, linked with `clang -arch x86_64`, and ran under Rosetta 2. 1282 of 1286 compiled, assembled, linked, and produced the expected exit code and stdout. The 4 failures are link errors from glibc-only symbol names in `test/include` stubs: `test_assert_fail__134`, `test_assert_pass__0`, `test_errno_einval__1` (`__errno_location` vs `__error`), and `test_stdio_streams__1` (`stderr` vs `__stderrp`). These are out of scope for this stage.
- Spec verification script: `_main:` present, no `.note.GNU-stack`, exit 42 under Rosetta 2. Auto-detect output matched the explicit `--target=x86_64-macos` output. `--target=x86_64-linux` produced `main:` and the footer. `--target=arm64-macos` and an unknown target both exited with status 1.
- Targeted symbol check: `_fp: dq _f`, `_s: dq Lstr0`, `_pa: dq _arr + 4`, block-scope `Lstatic_main_2: dq _arr + 8`, and `call _puts`. The program exited with 43 as expected.
- Standard test suite on macOS: 250 passed, 1911 failed. This matches the pre-change baseline; the failures are host issues (no `timeout`, ELF-only runners), deferred to Phase 0C.

### Step 7: Documentation

- `docs/outlines/checklist.md`: new `## Stage 179` section before `## TODO`. No TODO item is satisfied by this stage, so the TODO section is unchanged.
- `README.md`: intro names Mach-O64/macOS; usage synopsis and option list include `--target`; bootstrap table row describes the per-host assembler and linker; new "Through Stage 179" entry above "Through Stage 177".
- `docs/milestones/stage-179-milestone.md` and `docs/sessions/stage-179-transcript.md` added.
- `docs/grammar.md` not changed (no grammar change).

## Final Results

- **Build**: Successful. `bin/ccompiler` rebuilt with `--target` support.
- **Tests**: No new tests. Portable suite totals are unchanged from stage-177 (2161 portable, 194 system-include, 2 optional-library, 5 build-tool). The standard suite was run on this macOS host: 250 passed, 1911 failed, identical to the pre-change baseline. The failures are host issues (no `timeout`, ELF-only runners) deferred to Phase 0C.
- **Regression**: Linux `.asm` output is byte-identical to the pre-change compiler for all 1523 test and source files, except the expected `__TIME__` test. macOS end-to-end: 1282 of 1286 `test/valid` programs pass; the 4 failures are out-of-scope glibc-only symbol names in the test stubs.
- **Self-host**: Not performed on this host. `./build.sh --mode=self-host` must be run on a Linux host (or after Phase 0C) before the stage is considered fully verified.

## Session Flow

1. Read the stage-179 spec, the kickoff, and the stage-177 style references.
2. Resolved the three kickoff open items that required codegen decisions (shared helper prefix argument, `init_label_is_internal` flag, `__APPLE__` auto-detect acceptance).
3. Implemented target plumbing in `include/target.h` and `src/compiler.c`, then the `CodeGen` fields.
4. Implemented `sp(cg)` prefixing across `src/codegen.c` symbol sites and the footer guard.
5. Updated `build.sh` platform selection and `src/version.c`.
6. Built the compiler and ran the Linux byte-identical `.asm` regression against the pre-change compiler.
7. Ran macOS end-to-end checks under Rosetta 2, the spec verification script, and the targeted symbol check.
8. Ran the standard test suite on macOS and confirmed it matched the pre-change baseline.
9. Wrote the checklist, README, milestone, and transcript artifacts.

## Notes

- `__APPLE__` is not predefined by ClaudeC99's preprocessor (only `__linux__` is). A self-hosted `ccompiler` built on macOS therefore defaults to `x86_64-linux` when `--target` is omitted. This is accepted because `build.sh` always passes `--target` explicitly.
- `arm64-macos` is parsed and rejected after flag parsing, with the error raised once the target is resolved.
- VERSION_STAGE jumps from `01770000` to `01790000` because stage 178 did not bump it. This was left as-is rather than retroactively set to `01780000`.
- Spec line numbers had drifted; all prefix sites were re-derived from the code rather than the spec table.
- Out of scope: ARM64 codegen (Phase 1+), macOS test runner support (Phase 0C), and installing `nasm` (assumed via Homebrew).

## Commit

- Implementation: `04f878a` (stage 179: x86-64 Mach-O cross-compile target, Phase 0B).
