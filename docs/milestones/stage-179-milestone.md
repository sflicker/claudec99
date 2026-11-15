# Milestone Summary

## stage-179: x86-64 Mach-O Cross-Compile Target (Phase 0B) - Complete

stage-179 ships an `x86_64-macos` code generation target so the compiler, running on an Apple Silicon host, emits x86-64 NASM that assembles to Mach-O64 and runs under Rosetta 2.

- Tokenizer: No changes.
- Grammar/Parser: No changes (`docs/grammar.md` unchanged).
- AST/Semantics: No changes.
- Codegen: New `Target` enum (`include/target.h`) and `CodeGen.target_macho` flag. C-visible symbols (globals, functions, externs, `call` targets, `[rel ...]` references, `dq` address constants) get a `_` prefix on Mach-O via a `sp(cg)` helper; internal labels (`Lstr*`, `Lfc*`, `Lstatic_*`, `.L*`) are never prefixed. The `.note.GNU-stack` footer is omitted on Mach-O. Shared global load/store helpers gained an explicit prefix argument.
- Driver: `--target=x86_64-linux|x86_64-macos|arm64-macos` (unknown values are an error; `arm64-macos` parses but exits with "not yet implemented"). Target auto-detects from `__APPLE__` when omitted. Listed in `--help`.
- Build: `build.sh` selects `macho64` + `clang -arch x86_64` + `--target=x86_64-macos` on Darwin, and `elf64` + `gcc -no-pie` + `--target=x86_64-linux` elsewhere.
- Tests: No new tests (spec lists none). Linux `.asm` output is byte-identical to the pre-change compiler across all test sources, except the expected `__TIME__` test. 1282/1286 `test/valid` programs run end-to-end on macOS under Rosetta 2; the 4 failures are glibc-only symbol names in the test stub headers (out of scope). Portable suite totals are unchanged from stage-177.
- Docs: `VERSION_STAGE` set to `01790000`; README updated (target flag, bootstrap assembler/linker per host, Through stage-179 entry); checklist gains a Stage 179 section.
- Notes: The self-host (C0→C1→C2) run was not performed on this macOS host (no `timeout`, ELF-only test runners); it is deferred to a Linux host or Phase 0C. `__APPLE__` is never predefined by ClaudeC99's own preprocessor, so a self-hosted binary built on macOS defaults to the Linux target unless `--target` is passed; `build.sh` always passes it explicitly.
