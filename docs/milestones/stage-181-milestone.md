# Milestone Summary

## stage-181: ARM64 Codegen Skeleton + ARM64_TIER Gating (Phase 1) - Complete

stage-181 ships an experimental native ARM64 backend (`--target=arm64-macos`) that compiles a minimal subset of C to Apple Silicon assembly, with test gating through `ARM64_TIER`. The first native Apple Silicon binaries built by this compiler run without Rosetta.

- Tokenizer: No changes.
- Grammar/Parser: No changes (`docs/grammar.md` unchanged).
- AST/Semantics: No changes. Unsupported constructs are rejected by the ARM64 backend with `arm64-macos: <construct> not yet supported`; out-of-range integer literals are rejected with `arm64-macos: integer literal <v> out of range for this stage`.
- Preprocessor: `arm64-macos` predefines `__aarch64__`, `__arm64__`, `__APPLE__`, and `__MACH__` (not `__x86_64__`).
- Codegen: New `CodeGenArm64` backend (`include/codegen_arm64.h`, `src/codegen_arm64.c`) emits `.s` output for functions whose body is exactly one `return <integer literal 0..65535>`, using an AAPCS64 frame and `.globl _name`. Output path is `<name>.s` for arm64 via `make_output_path_ext`. The `-O2` peephole pass is skipped and `-g` is ignored for this target. x86-64 output is unchanged.
- Test infrastructure: `test/lib/platform.sh` gains `ASM_EXT`, an `assemble` helper (nasm or `clang -arch arm64 -c`), `LINK_CMD` for `clang -arch arm64`, `ARM64_TIER` (default 1), and `arm64_tier_includes`. `test/valid/run_tests.sh` gates tests by the manifest `test/valid/arm64_tiers.txt`. The integration suite reports SKIP under arm64 until tier 10.
- Tests: 2 new valid tests (return 0, return 255), 1 new host-independent integration test (`test_arm64_skeleton_asm`, covering `.s` output and both error paths), and an extended predefined-macro integration test. Native ARM64 tier 1: 5 of 1289 in scope, 5/5 pass. Full portable suite 2166/2166 pass (249 unit, 1289 valid, 261 invalid, 196 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- Docs: `VERSION_STAGE` set to `01810000`; README gains a Through Stage 181 entry, arm64-macos usage and `ARM64_TIER` test notes; checklist gains a Stage 181 section; roadmap and self-compilation report updated.
- Notes: Scope is Phase 1 only. Arithmetic, locals, and `movz`/`movk` immediates are deferred to stage 182. Linux regression is byte-identical for all test sources except the `__TIME__` test and new tests. Linux x86-64 self-host is pending. Compile-error line numbers are offset by about 5 lines in both backends (pre-existing, follow-up). macOS `ld` warnings remain.
