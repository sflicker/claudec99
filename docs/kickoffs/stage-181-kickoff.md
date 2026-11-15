# Stage 181 Kickoff: ARM64 Codegen Skeleton + `ARM64_TIER` Gating (Phase 1)

## Derived Values

| Item | Value |
|------|-------|
| STAGE_LABEL | stage-181 |
| STAGE_DISPLAY | stage-181 |
| Spec | `docs/stages/ClaudeC99-spec-stage-181-arm64-skeleton.md` |
| New backend | `include/codegen_arm64.h`, `src/codegen_arm64.c` (`CodeGenArm64`, `codegen_arm64_init` / `_translation_unit` / `_free`) |
| New output helper | `make_output_path_ext(char *out, size_t n, const char *input, const char *ext)` in `include/util.h` / `src/util.c` (`make_output_path` delegates with `".asm"`) |
| New manifest | `test/valid/arm64_tiers.txt` (tier 1: five entries) |
| New tests | `test/valid/misc/test_return_zero__0.c`, `test/valid/misc/test_return_255__255.c`, `test/integration/test_arm64_skeleton_asm/` |
| Extended test | `test/integration/test_target_predefined_macros` (arm64-macos check, `has_aarch64` / `has_arm64`) |
| Test lib changes | `test/lib/platform.sh`: `ASM_EXT`, `assemble`, `ARM64_TIER`, `arm64_tier_includes`, `LINK_CMD` for arm64 |
| VERSION_STAGE | `01810000` (stage 180 set `01800000`) |
| Expected output | Linux `.asm` byte-identical to stage 180 (except `__TIME__` test and new tests); native `TARGET=arm64-macos` valid run 5/5; macOS default suite 2166; macOS self-host C0→C1→C2 passes |

## 1. Spec Summary

Stage 181 is Phase 1 of the ARM64/macOS roadmap. It starts the native AArch64 backend for `--target=arm64-macos` and adds per-tier test gating so `test/valid` can run against the ARM64 backend one tier at a time.

The backend is a skeleton. It compiles only programs whose `main` is a bare `return <integer-literal>;`, with the literal in `0..65535`, to Apple/GNU-syntax ARM64 assembly in `<name>.s`. `clang -arch arm64` assembles and links it, and it runs natively with the expected exit code. Everything else is a compile error:

```
error: arm64-macos: <construct> not yet supported
```

The `x86_64-linux` and `x86_64-macos` targets must not change. Auto-detection is unchanged: macOS still defaults to `x86_64-macos` until Phase 10.

### Planned changes

- **Part A, ARM64 backend (new files):** `include/codegen_arm64.h` and `src/codegen_arm64.c`. `CodeGenArm64` holds only `FILE *output` and shares no state with `CodeGen`. The translation unit may contain `AST_TYPEDEF_DECL` (ignored), prototypes (`AST_FUNCTION_DECL` without a body, ignored), and functions whose body is an `AST_BLOCK` with exactly one `AST_RETURN_STATEMENT` returning an `AST_INT_LITERAL`. The literal is parsed with `strtol` base 0, so `00` works. Symbols get the `_` prefix and `.globl` unless the function is `static`. Every function uses the full AAPCS64 frame (`stp x29, x30, [sp, #-16]!` / `mov x29, sp` / `mov w0, #N` / `ldp x29, x30, [sp], #16` / `ret`). Unsupported constructs use a small local AST-type→name switch, since no existing helper exists. Literals outside `0..65535` give `error: arm64-macos: integer literal <v> out of range for this stage`.
- **Part B, `src/compiler.c`:** remove the "target 'arm64-macos' is not yet implemented" error. Output is `<name>.s` for arm64 and `<name>.asm` for x86. `compile_one_file` dispatches `arm64-macos` to `CodeGenArm64` and every other target to the unchanged `CodeGen`. Under arm64, `-O2` skips the x86 peephole pass (`-O1` still runs) and `-g` is accepted silently. `--help` lists `arm64-macos (experimental)`.
- **Part C, `src/preprocessor.c`:** `arm64-macos` predefines `__aarch64__`, `__arm64__`, `__APPLE__`, `__MACH__`. It does not define `__x86_64__`, `__linux__`, or `__unix__`. LP64 and `__SIZEOF_*__` values are unchanged.
- **Part D, test infrastructure:**
  - `test/lib/platform.sh`: `TARGET=arm64-macos` is valid on Darwin only (otherwise `TARGET=arm64-macos requires a macOS host`). Adds `ASM_EXT` (`asm` / `s`), `assemble <in> <out> [flags]` (`nasm` for x86, `clang -arch arm64 -c` for arm64, extra flags ignored), `LINK_CMD` = `clang -arch arm64`, `ARM64_TIER` (default `1`, environment-overridable), and `arm64_tier_includes <relpath>`.
  - `test/valid/arm64_tiers.txt`: manifest format `<tier> <path relative to test/valid>`. Files or directory prefixes. Comments and blank lines ignored.
  - `test/valid/run_tests.sh`: under arm64, skips tests not in scope. Skipped tests are not counted in `total`. Prints `ARM64_TIER=<n>: <k> of <N> tests in scope` before the summary.
  - `test/integration/run_tests.sh`: under arm64, skips the whole suite, printing `SKIP  integration suite (TARGET=arm64-macos; enabled at tier 10)` and `Results: 0 passed, 0 failed, 0 total`.
  - All assembling runners and custom `run_test.sh` scripts use `ASM_EXT` and `assemble` instead of hardcoded `.asm` and `nasm`.
  - Unchanged: `test/invalid`, `test/print_ast`, `test/print_tokens`, `test/print_asm` (pinned to `x86_64-linux`).
- **Part E, `src/version.c`:** `VERSION_STAGE "01810000"`.
- **Build files:** `src/codegen_arm64.c` is added to `build.sh` `SRC_FILES` and `CMakeLists.txt`. It must self-compile.
- **Roadmap:** Phase 1 now includes `ARM64_TIER` gating. Tier 1 is "manifest-listed `return <literal>` programs" instead of "simple `statements/`".

## 2. Required Tokenizer Changes

None.

## 3. Required Parser Changes

None. `docs/grammar.md` stays as is.

## 4. Required AST Changes

None. The backend consumes the existing AST. Verified facts: bodies appear as an `AST_BLOCK` child of `AST_FUNCTION_DECL`, prototypes appear as `AST_FUNCTION_DECL` without a block, and the translation unit always begins with two preamble `AST_TYPEDEF_DECL` nodes.

## 5. Required Code Generation Changes

- New `CodeGenArm64` backend (`src/codegen_arm64.c`), dispatched only for `arm64-macos`.
- `compile_error()` is the error path for unsupported constructs and out-of-range literals.
- `make_output_path_ext` added to `util`. Existing `make_output_path` behaviour unchanged.
- No changes to `src/codegen.c`, the peephole pass, or x86 output.

Expected output for `int main(void) { return 42; }`:

```asm
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.p2align	2
_main:
	stp	x29, x30, [sp, #-16]!
	mov	x29, sp
	mov	w0, #42
	ldp	x29, x30, [sp], #16
	ret
```

Exact whitespace is not part of the contract.

## 6. Test Plan

### New tests

- **(a) `test/valid/misc/test_return_zero__0.c`:** `int main(void) { return 0; }`. Target-neutral.
- **(b) `test/valid/misc/test_return_255__255.c`:** `int main(void) { return 255; }`. Target-neutral.
- **(c) `test/integration/test_arm64_skeleton_asm/`** (custom `run_test.sh`, text-only, host-independent, runs on Linux too):
  - `return 42` under `--target=arm64-macos` produces `<name>.s` and no `<name>.asm`. Whitespace-normalized output contains `.globl _main`, `_main:`, `mov w0, #42`, `ret`.
  - A local variable under `--target=arm64-macos` exits 1 with stderr containing `arm64-macos:` and `not yet supported`.
  - `return 70000` under `--target=arm64-macos` gives the out-of-range error.
- **(d) Extended `test/integration/test_target_predefined_macros`:** arm64-macos check. Expect surviving markers `has_apple has_mach` plus `has_aarch64` / `has_arm64`, and no `has_x86_64`. The x86 expectations are unchanged.

Tests (a) and (b) join the default suite totals and pass under both x86 targets.

### Verification

1. **Native ARM64 (Apple Silicon):** `TARGET=arm64-macos ./test/valid/run_tests.sh` reports `ARM64_TIER=1: 5 of <N> tests in scope` and `Results: 5 passed, 0 failed, 5 total`.
2. **macOS default target:** `./test/run_all_tests.sh` passes the full portable suite at 2166 (2163 + 2 valid + 1 integration).
3. **Linux regression:** generated `.asm` for every test source under `--target=x86_64-linux` is byte-identical to stage 180, except the `__TIME__` test and the new tests.
4. **macOS self-host:** `./build.sh --mode=self-host` passes C0→C1→C2 on the `x86_64-macos` path, including the new `codegen_arm64.c`.
5. **Linux self-host:** remains pending, as in stage 180. Record the status in the milestone.

## Findings

No spec ambiguities were found. Verified facts:

1. No existing AST-type→name helper exists. A small local switch in `codegen_arm64.c` is the chosen approach.
2. Function bodies are detected as an `AST_BLOCK` child of `AST_FUNCTION_DECL`. Prototypes are `AST_FUNCTION_DECL` with no block.
3. The translation unit always starts with two preamble `AST_TYPEDEF_DECL` nodes, which the backend ignores.
4. `compile_error()` is the codegen error path.

## Implementation Plan

Order:

1. `make_output_path_ext` in `include/util.h` / `src/util.c`, and the output-path change in `src/compiler.c`
2. `include/codegen_arm64.h`, `src/codegen_arm64.c`
3. Dispatch in `compile_one_file` (`src/compiler.c`), `-O2` / `-g` handling, `--help`
4. Predefined macros (`src/preprocessor.c`)
5. `build.sh` `SRC_FILES`, `CMakeLists.txt`
6. `src/version.c`
7. `test/lib/platform.sh`, `test/valid/arm64_tiers.txt`, runners
8. New tests (a)–(d)
9. Verification (native ARM64 run, macOS default suite, Linux `.asm` regression)
10. macOS self-host (C0→C1→C2); record Linux self-host as pending
11. Docs (milestone, transcript, session export, README, roadmap)
12. Commit (only when the user asks)

Pause after each step for review.

## Notes

- Stage 180 reserved `TARGET=arm64-macos` as an error in `platform.sh`. Stage 181 makes it valid, on Darwin only.
- Out of scope: any ARM64 construct beyond `return <literal 0..65535>` (arithmetic, locals, `movz`/`movk`, control flow, calls, globals), switching macOS auto-detection (Phase 10), an ARM64 peephole pass, ARM64 DWARF / `-g`, ARM64 `print_asm` expected files, `bin/cc99` macOS/ARM64 support, and the macOS `ld` warnings.
- This kickoff is planning only. It makes no compiler source, test, runner, or `build.sh` changes.
