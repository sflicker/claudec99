# Milestone Summary

## Stage 171: --help, Verbose Mode, and Mixed Input File Types - Complete

Stage 171 ships three CLI surface improvements (no tokenizer, parser, AST, or codegen changes): `--help` flag support for both `ccompiler` and `bin/cc99`, verbose mode (`-v`/`--verbose`) with a new `g_verbose` global controlling progress output visibility, and extended `bin/cc99` input file support for pre-assembled (`.s`/`.asm`), pre-compiled (`.o`), and library (`.a`/`.so`) files.

- Tokenizer: no changes.
- Grammar/Parser: no changes.
- AST/Semantics: no changes.
- Codegen: no changes.
- Compiler (`ccompiler`): Added `extern int g_verbose;` to `include/util.h` and `int g_verbose = 0;` to `src/util.c`; added `--help` branch printing formatted help to stdout with exit 0; added `-v`/`--verbose` branch setting `g_verbose = 1`; wrapped `compiled: X -> Y` progress line in conditional `if (g_verbose)` guard; simplified error-path usage string to redirect to `--help`.
- Wrapper script (`bin/cc99`): Added `--help` case with formatted help listing input file types and all options; added `-v`/`--verbose` case setting `verbose` flag and forwarding `-v` to compiler; added `run_cmd` helper that prints each tool invocation (ccompiler, nasm, gcc) to stderr when verbose; extended input file classification to accept `.s`/`.asm` (skips compile, assembles with nasm), `.o` (skips compile and assemble, routes to linker), and `.a`/`.so` (added to link_flags directly); refactored compilation loop with extension dispatch to determine pipeline entry point (compile step needed for `.c` only).
- Tests: No new tests required (existing suite exercises only `.c` inputs). Full suite: 2072 portable (165 unit, 1286 valid, 261 invalid, 189 integration, 50 print-AST, 100 print-tokens, 21 print-asm) + 189 system-include + 2 optional-library pass.
- Docs: Updated `docs/outlines/checklist.md` to mark Stage 171 complete; updated `README.md` to reflect Stage 171 as current stage and document new CLI features.
- Notes: All three features are pure tooling; no language or codegen feature work. `--help` output format and `run_cmd` invocation style match standard compiler conventions (similar to GCC/Clang). Mixed input support enables `cc99` to function as a drop-in replacement for `gcc` in typical build workflows.
