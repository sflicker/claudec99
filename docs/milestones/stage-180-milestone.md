# Milestone Summary

## stage-180: macOS-Aware Test Infrastructure (Phase 0C) - Complete

stage-180 ships macOS-aware test runners and preprocessor target handling so the full portable suite runs on an Apple Silicon host and the self-host cycle (C0→C1→C2) passes on macOS ARM64.

- Tokenizer: No changes.
- Grammar/Parser: No changes (`docs/grammar.md` unchanged).
- AST/Semantics: No changes.
- Preprocessor: `preprocess_set_target(Target)` predefines `__APPLE__`/`__MACH__` for `x86_64-macos` (instead of `__linux__`/`__unix__`), so self-hosted macOS binaries auto-detect the correct target.
- Codegen: On Mach-O, `extern` data is accessed through the GOT (`mov r11, [rel _x wrt ..gotpcrel]`), because dylib data such as `__stdoutp` is not RIP-reachable. Linux output is byte-identical.
- Test infrastructure: New `test/lib/platform.sh` is the shared runner config (host detection, `TARGET` override, NASM format, link command, timeout wrapper). Valid, integration, invalid, print-AST, print-tokens, print-asm, and the custom multi-file integration runners source it. `build.sh` uses the same timeout wrapper. Test stub headers gain `__APPLE__` branches for `stdio.h` and `errno.h`.
- Tests: 2 new tests (one valid preprocessor test, one host-independent integration test for predefined macros); 2 existing preprocessor tests gained an `__APPLE__` pass-through. Full portable suite 2163/2163 pass on macOS ARM64 (249 unit, 1287 valid, 261 invalid, 195 integration, 50 print-AST, 100 print-tokens, 21 print-asm), up from 250 passed / 1911 failed before this stage.
- Docs: `VERSION_STAGE` set to `01800000`; README gains a Through Stage 180 entry and updated self-host notes; checklist gains a Stage 180 section.
- Notes: Linux x86-64 self-host is pending (not runnable from this host). System-include, optional-library, and build-tool suites are Linux-only and were not run here. `TARGET=arm64-macos` is rejected until Stage 181 (`ARM64_TIER`). Harmless `ld` warnings remain on macOS link (`pointer not aligned`, `no platform load command`).
