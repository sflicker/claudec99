# stage-181 Transcript: ARM64 Codegen Skeleton + ARM64_TIER Gating

## Summary

Stage 181 adds the first native ARM64 backend, selected with `--target=arm64-macos`. The backend is experimental and deliberately narrow: a function is accepted only if its body is exactly one `return <integer literal>` with a value in 0..65535. The backend emits an AAPCS64 frame, a `.section __TEXT,__text` header, `.globl _name` for non-static functions, and writes `<name>.s`. Assembly and linking use `clang -arch arm64`. Any other construct (locals, parameters, globals, multi-statement bodies, empty bodies, arithmetic) is rejected with `arm64-macos: <construct> not yet supported`, and an out-of-range literal is rejected with `arm64-macos: integer literal <v> out of range for this stage`.

The stage also introduces `ARM64_TIER`, a test gating mechanism. The manifest `test/valid/arm64_tiers.txt` lists the tier-1 tests (five return-only programs). Under `TARGET=arm64-macos`, the valid runner counts only in-scope tests and prints `ARM64_TIER=<n>: <k> of <N> tests in scope`. The integration suite prints SKIP and 0/0/0 under arm64 until tier 10. The old `arm64-macos: not yet implemented` error in the driver is removed.

Constraints: no tokenizer, parser, AST, or grammar changes. The preprocessor predefines `__aarch64__` and `__arm64__` for the new target. Linux and x86-64 macOS output is unchanged. This is Phase 1 only; arithmetic, locals, and `movz`/`movk` immediates are deferred to stage 182.

## Plan

The kickoff (`docs/kickoffs/stage-181-kickoff.md`) set the implementation order as runner and platform config first, then the backend, then driver and preprocessor wiring, then tests and verification. During implementation, the local-variable case first reported `multi-statement function body`. It was changed to report the first non-return statement by its construct name (`local variable declaration`), which matches the spec's error wording more closely.

## Changes Made

### Step 1: Codegen Backend

- Added `include/codegen_arm64.h` declaring `CodeGenArm64` with an `output` stream and `codegen_arm64_init`, `codegen_arm64_translation_unit`, and `codegen_arm64_free`.
- Added `src/codegen_arm64.c`: skips `AST_TYPEDEF_DECL` and bodiless prototypes; requires exactly one `return <AST_INT_LITERAL>` per function body, parsed with `strtol` base 0.
- Emits `.section __TEXT,__text,regular,pure_instructions`, `.globl _name` (unless static), `.p2align 2`, and the frame `stp x29, x30, [sp, #-16]!` / `mov x29, sp` / `mov w0, #N` / `ldp x29, x30, [sp], #16` / `ret`.
- Adds a local AST-type-to-name switch for unsupported-construct messages and an `arm64_mark` helper for error positions that mirrors `cg_mark`.

### Step 2: Driver and Output Path

- `src/compiler.c`: accepts `arm64-macos`, dispatches codegen by target, skips `-O2` peephole and ignores `-g` for arm64, and lists `arm64-macos (experimental)` in `--help`.
- `include/util.h`, `src/util.c`: added `make_output_path_ext`; `make_output_path` delegates with `.asm`. arm64 output uses `.s`.
- macOS auto-detection is unchanged (`x86_64-macos`).

### Step 3: Preprocessor and Build Wiring

- `src/preprocessor.c`: `arm64-macos` predefines `__aarch64__`, `__arm64__`, `__APPLE__`, and `__MACH__`, and omits `__x86_64__`.
- `CMakeLists.txt` and `build.sh`: added `src/codegen_arm64.c` to the source lists.
- `src/version.c`: `VERSION_STAGE` set to `01810000`.

### Step 4: Test Platform and Tier Gating

- `test/lib/platform.sh`: allows `TARGET=arm64-macos` on Darwin only; sets `ASM_EXT` (`asm` or `s`); adds `assemble <in> <out> [nasm flags]` (nasm or `clang -arch arm64 -c`); sets `LINK_CMD` to `clang -arch arm64`; defines `ARM64_TIER` (default 1) and `arm64_tier_includes <relpath>`.
- New `test/valid/arm64_tiers.txt` with five tier-1 entries.
- `test/valid/run_tests.sh`: tier-gates under arm64 and prints the in-scope count.
- `test/valid/run_test.sh`, `test/integration/run_tests.sh`, `test/integration/run_test.sh`, and the four custom multi-file scripts: use `ASM_EXT` and `assemble`.
- `test/integration/run_tests.sh`: prints SKIP and 0/0/0 under arm64.

### Step 5: Tests

- New `test/valid/misc/test_return_zero__0.c` and `test/valid/misc/test_return_255__255.c`.
- New `test/integration/test_arm64_skeleton_asm/`: `return42.c`, `local_var.c`, `big_literal.c`, and `run_test.sh`. The script is text-only and host-independent. It checks that `.s` is produced and `.asm` is not, that `.globl _main`, `_main:`, `mov w0, #42`, and `ret` appear, and that the local-variable and big-literal error paths report the expected messages.
- `test_target_predefined_macros`: extended with `has_aarch64` and `has_arm64` markers and an arm64-macos check.

### Step 6: Docs and Checkpoints

- Added the Stage 181 section to `docs/outlines/checklist.md`, the milestone and transcript, and the README Through Stage 181 entry and target/tier notes.
- Self-host checkpoints: C0 `82e47d2`, C1 `37c9ee3`. Self-host report `4e1834c`.

## Final Results

- **Build**: Successful. `VERSION_STAGE` is `01810000`.
- **Native ARM64 (`TARGET=arm64-macos ARM64_TIER=1`)**: `ARM64_TIER=1: 5 of 1289 tests in scope`, 5/5 pass. These are the first native Apple Silicon binaries from this compiler, run without Rosetta.
- **Portable (macOS, x86_64-macos under Rosetta 2)**: 2166/2166 pass (249 unit, 1289 valid, 261 invalid, 196 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Up from 2163 at stage 180.
- **Self-host (macOS ARM64)**: C0 `00.03.01810000.01314` (AppleClang), C1 `.01315` (built by C0), C2 `.01316` (built by C1). Each stage passes 2166/2166. A C1 built through the bootstrap produced ARM64 code for `return 42` that ran natively with exit code 42.
- **Regression**: Linux-target `.asm` is byte-identical to stage 180 for all test sources, except the `__TIME__` test and the new tests. The compiler's own `src/*.asm` differ only by an added `extern make_output_path_ext` line in files that include `util.h`.
- **Not run**: Linux x86-64 self-host (pending).

## Session Flow

1. Read the stage-181 spec, the kickoff, and the stage-180 milestone and transcript style references.
2. Implemented the ARM64 backend header and source, with the unsupported-construct and literal-range diagnostics.
3. Wired the driver, output-path helper, preprocessor target macros, and build lists.
4. Added the platform config, `ARM64_TIER` manifest, and runner tier gating.
5. Added the valid and integration tests, and extended the predefined-macro test.
6. Built and ran the native ARM64 tier-1 suite, then the macOS portable suite to 2166/2166.
7. Ran the Linux `.asm` byte-identity regression.
8. Ran the macOS ARM64 self-host cycle (C0, C1, C2), each stage 2166/2166.
9. Wrote the milestone, transcript, checklist, and README artifacts.

## Notes

- Compile-error line numbers are offset by about 5 lines (for example, `file.c:7` for line 2) in both backends. This is pre-existing and likely caused by the injected preamble. It is not fixed in this stage.
- macOS `ld` warnings remain (`pointer not aligned`, `no platform load command`) and are harmless.
- Follow-ups: Linux x86-64 self-host verification; `bin/cc99` macOS support; Phase 2 (stage 182) arithmetic, locals, and `movz`/`movk` immediates.

## Commit

- Implementation: `35a898b` (stage 181: ARM64 codegen skeleton + ARM64_TIER gating, Phase 1).
- Self-host checkpoints: `82e47d2` (C0), `37c9ee3` (C1).
- Self-host report: `4e1834c` (docs: stage 181 self-compilation report, macOS ARM64 self-host).
