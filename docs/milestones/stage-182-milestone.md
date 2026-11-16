# Milestone Summary

## stage-182: ARM64 Integer Arithmetic and Local Variables (Phase 2) - Complete

stage-182 extends the experimental native ARM64 backend (`--target=arm64-macos`) from a single `return <literal>` to straight-line signed integer code in parameterless functions.

- Tokenizer: No changes.
- Grammar/Parser: No changes (`docs/grammar.md` unchanged).
- AST/Semantics: No changes. The backend consumes the existing AST and rejects unsupported constructs with `arm64-macos: <construct> not yet supported`, including comparisons, logical operators, unsigned and char/short types, globals, static/extern locals, parameters, and return without a value. The stage 181 out-of-range literal error is removed.
- Codegen: `CodeGenArm64` (`include/codegen_arm64.h`, `src/codegen_arm64.c`) now handles `int` and `long` (`long long`) locals, block scoping with shadowing, arithmetic, bitwise, shift, unary, assignment, prefix/postfix `++`/`--`, and comma. Values flow through a stack machine in `w0`/`x0`. Large constants use `movz`/`movk`. Each declaration gets its own frame slot (16-byte-rounded frame, 4095-byte limit). Fall-through returns 0 from `main`, and empty bodies are accepted. x86 targets are unchanged.
- Tests: 4 new valid tests (large literals, long-to-int truncation, nested block shadowing, falling off the end of `main`). The ARM64 tier-2 manifest gains 68 entries. The integration test `test_arm64_skeleton_asm` now checks `movz`/`movk` output and uses `return 1 < 2;` as the unsupported-construct case. Native ARM64 tier 2: 73 of 1293 in scope, 73/73 pass. Full portable suite 2170/2170 pass (249 unit, 1293 valid, 261 invalid, 196 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- Docs: `VERSION_STAGE` set to `01820000`; README gains a Through Stage 182 entry and an updated `--target` note; checklist gains a Stage 182 section; self-compilation report updated.
- Notes: Scope is Phase 2 only. Control flow (Phase 3), calls and parameters (Phase 4), and unsigned arithmetic (Phase 7) remain deferred. The parser does not record `unsigned` on function return types, so unsigned returns cannot be rejected by the backend; the result is harmless because the conversion is sign extension. Linux `.asm` output is byte-identical to stage 181 for all test sources (1706 with output, 0 differences). macOS ARM64 self-host C0→C1→C2 passes (each 2170/2170). Linux x86-64 self-host is pending.
