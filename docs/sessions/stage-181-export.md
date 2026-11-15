# Session Export — stage-181: ARM64 Codegen Skeleton + `ARM64_TIER` Gating (Phase 1)

**Date:** 2026-11-15
**Host:** macOS ARM64 (Darwin 25.5.0), Apple clang 21.0.0, NASM 3.02 (Homebrew), Rosetta 2
**Spec:** `docs/stages/ClaudeC99-spec-stage-181-arm64-skeleton.md`

**Commits:**

| Commit | What |
|--------|------|
| `73c99e7` | spec and roadmap update (planning) |
| `35a898b` | code |
| `82e47d2`, `37c9ee3` | self-host checkpoints, committed by `build.sh` |
| `4e1834c` | self-compilation report |
| `0292f4c` | docs |

---

## 1. Planning (before `/implement-stage`)

- **Codebase findings:**
  - Only 3 `test/valid` programs are a bare `return <literal>;` main: `misc/test_return__42`, `control_flow/test_return__42` and `arithmetic/test_octal_zero__0`.
  - The roadmap's tier 1 ("simple `statements/`") doesn't match the tree: that directory holds 6 switch tests.
  - Output was always `<name>.asm`, and clang won't treat `.asm` as assembly source.
- **User decisions:**
  - Emit `<name>.s` for `arm64-macos`; the x86 targets keep `.asm`.
  - Gate tests with a manifest, `test/valid/arm64_tiers.txt` (`<tier> <file-or-dir>`), rather than a directory table hardcoded in the runner.
- **Spec written:**
  - Part A: backend files
  - Part B: `compiler.c` dispatch and `.s` output
  - Part C: ARM64 predefined macros
  - Part D: `platform.sh` `ASM_EXT`, `assemble` and tier gating, plus the runners
  - Part E: version
  - Tests and acceptance criteria
- **Roadmap:** tier 1 redefined; Phase 1 scope note added. Committed as `73c99e7`.

## 2. `/implement-stage ClaudeC99-spec-stage-181-arm64-skeleton`

`STAGE_DISPLAY: stage-181`

### Pre-implementation checks

- A function body is an `AST_BLOCK` child of `AST_FUNCTION_DECL`; a prototype has no block.
- Every translation unit starts with two `AST_TYPEDEF_DECL` nodes from the preamble.
- There is no existing helper that names AST node types, so a local switch is needed.
- Codegen reports errors through `compile_error()`; positions come from `src_line`/`src_col` (see `cg_mark`).
- No spec ambiguities.

### Kickoff

`docs/kickoffs/stage-181-kickoff.md` (`haiku-stage-artifact-writer`).

## 3. Implementation

A Stage 180 Linux `.asm` baseline (1525 files) was captured first, after a clean native rebuild. `build/ccompiler` had been the Rosetta C2 binary left by the previous self-host.

| File | Change |
|------|--------|
| `include/util.h`, `src/util.c` | `make_output_path_ext(out, n, input, ext)`; `make_output_path` delegates with `".asm"` |
| `include/codegen_arm64.h`, `src/codegen_arm64.c` (new) | `CodeGenArm64`; the Phase 1 backend described below |
| `src/compiler.c` | `arm64-macos` accepted; `.s` output; codegen dispatch; `-O2` peephole skipped / `-g` ignored for arm64; `--help` lists `arm64-macos` (experimental) |
| `src/preprocessor.c` | `arm64-macos` predefines `__aarch64__`/`__arm64__` (not `__x86_64__`) plus `__APPLE__`/`__MACH__` |
| `CMakeLists.txt`, `build.sh` | added `src/codegen_arm64.c` |
| `src/version.c` | `01810000` |
| `test/lib/platform.sh` | `TARGET=arm64-macos` (Darwin only); `ASM_EXT`; `assemble`; arm64 `LINK_CMD`; `ARM64_TIER` (default 1); `arm64_tier_includes` |
| `test/valid/arm64_tiers.txt` (new) | tier 1: five `return <literal>` programs |
| `test/valid/run_tests.sh` | tier gating under arm64 (out-of-scope tests not counted, "ARM64_TIER=n: k of N tests in scope"); `ASM_EXT`/`assemble` |
| `test/valid/run_test.sh`, `test/integration/run_test.sh`, 4 custom multi-file scripts | `ASM_EXT`/`assemble` |
| `test/integration/run_tests.sh` | `ASM_EXT`/`assemble`; whole suite skipped under arm64 (enabled at tier 10) |
| Tests | new `test_return_zero__0`, `test_return_255__255`; new `test_arm64_skeleton_asm/` (text-only); `test_target_predefined_macros` extended with arm64 checks |

The new backend in `src/codegen_arm64.c`:

- Skips typedefs and prototypes.
- Accepts a function only if its body is one `return <int literal 0..65535>`.
- Emits `.section __TEXT,__text`, `.globl _name` (unless the function is `static`) and the AAPCS64 frame:

  ```
  stp x29,x30,[sp,#-16]!
  mov x29,sp
  mov w0,#N
  ldp x29,x30,[sp],#16
  ret
  ```

- Reports two kinds of error:
  - `arm64-macos: <construct> not yet supported`
  - `integer literal <v> out of range for this stage`

### Session notes

- I first wrote a bulk `perl` in-place edit to `src/codegen_arm64.c`. You rejected that tool call, and the rest of the stage's source and script edits were made with the Edit tool.
- The first error check reported a local variable as "multi-statement function body". I changed it to report the first statement that isn't a return, by construct (`local variable declaration`).
- I observed, but did not change, that compile-error line numbers are about 5 lines too high (`file.c:7` for line 2) in **both** backends. The injected preamble is the likely cause. Logged as a follow-up.

## 4. Verification

| Check | Result |
|-------|--------|
| First native ARM64 binary | `return 42` → `.s` → `clang -arch arm64` → Mach-O arm64, exit 42 (no Rosetta) |
| Error paths | local variable, out-of-range literal, global variable, binary expression, empty and multi-statement bodies all rejected with the expected messages; `static` functions get no `.globl`; `-O2 -g` accepted |
| `TARGET=arm64-macos ./test/valid/run_tests.sh` | `ARM64_TIER=1: 5 of 1289 tests in scope`, **5/5 pass** |
| `ARM64_TIER=0` / arm64 integration | 0 of 1289 in scope / suite skipped (0/0/0) |
| Default macOS suite (`x86_64-macos`, Rosetta) | **2166/2166**: 249 unit, 1289 valid, 261 invalid, 196 integration, 50 print-AST, 100 print-tokens, 21 print-asm |
| Linux regression | test-source `.asm` byte-identical to stage 180 (only the `__TIME__` test and new tests differ); compiler `src/*.asm` differ only by an added `extern make_output_path_ext` line where `util.h` is included |
| Quick bootstrap | `codegen_arm64.c` self-compiles; the C1 it built (x86_64) produced ARM64 `return 42` that ran natively with exit 42 |
| Self-host (`--mode=self-host`, macOS) | **C0, C1 and C2 each passed 2166/2166** (versions below) |

Self-host versions:

| Step | Version | Built by |
|------|---------|----------|
| C0 | `00.03.01810000.01314` | AppleClang |
| C1 | `00.03.01810000.01315` | C0 |
| C2 | `00.03.01810000.01316` | C1 |

## 5. Documentation

- `docs/self-compilation-report.md`: stage 181 macOS self-host entry; no bootstrap issues (`4e1834c`).
- `docs/outlines/arm64-macos-backend-roadmap.md`: Phase 1 checked off; "Next up" is now Phase 2 (stage 182).
- Written by `haiku-stage-artifact-writer`:
  - `docs/milestones/stage-181-milestone.md`
  - `docs/sessions/stage-181-transcript.md`
  - `docs/outlines/checklist.md`: Stage 181 section; no TODO item completed.
  - `README.md`:
    - new "Through Stage 181" entry
    - the `--target` bullet describes `arm64-macos` (experimental)
    - Tests note on `ARM64_TIER` and the manifest
- I made one small README formatting fix: code formatting for `arm64-macos` in the `--target` bullet.
- `docs/grammar.md`: unchanged, since the grammar did not change.

## 6. Follow-ups

- Compile-error line numbers about 5 lines too high (both backends, likely the preamble).
- Run the Linux x86-64 self-host (stages 180 and 181).
- macOS `ld` warnings: data pointer alignment, and the missing platform load command.
- `bin/cc99` macOS/ARM64 support.
- The Stage 179 and 180 README entries mention `arm64-macos` as unimplemented; accurate for their stages, left unchanged.
- Stage 182 (Phase 2): ARM64 integer arithmetic, local variables and `movz`/`movk` immediates.
