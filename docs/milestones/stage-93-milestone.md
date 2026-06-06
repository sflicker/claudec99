# Milestone Summary

## Stage 93: Bootstrap Build Workflow - Complete

stage-93 ships a multi-mode build system that enables normal, bootstrap, and fallback build strategies, plus timeout guards and build-by attribution in version output.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: No changes.
- Build System: New `build.sh` wrapper script with three modes: `--mode=normal` (cmake default), `--mode=bootstrap` (self-compile via pre-built ccompiler), `--mode=fallback` (bootstrap if available, normal otherwise). Bootstrap mode includes per-file `--timeout` (default 300s) to guard against infinite loops. `CMakeLists.txt` computes `BUILTBY_TOKEN` from compiler ID/version for version output attribution.
- Version: `VERSION_STAGE` updated to "00930000". Added `VERSION_BUILTBY` macro support (default `DefaultCcompiler` token stringified via C99 `#` operator). Version output extended to two lines with `BuiltBy:` field. version_buf expanded 128 → 256 bytes.
- Tests: Five test suites augmented with `TIMEOUT=${CLAUDEC99_TEST_TIMEOUT:-30}` variable and timeout-wrapped compiler invocations/execution. All 1306 tests pass (823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). No regressions.
- Docs: README.md updated to reflect stage 93 completion and describe the bootstrap workflow capability.
- Notes: Spec fixes applied: "Fallout back Build" → "Fallback Build"; `VERSION_BUILTBY` default changed to use C99 stringification macro (not valid C string); output field name corrected to "BuiltBy:" (capital B, not "BuildBy:"); timeout values chosen as 30s per test, 300s per bootstrap compilation.
