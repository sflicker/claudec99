# Milestone Summary

## Stage 170: Warning Level Flags (-Wall, -Wextra) - Complete

stage-170 ships `-Wall` and `-Wextra` flag support as pure infrastructure, accepting both flags without error and storing them in a global `g_warn_level` (0=none, 1=Wall, 2=Wall+Wextra). No new warning diagnostics are emitted in this stage.

- Tokenizer: no changes.
- Grammar/Parser: no changes.
- AST/Semantics: no changes.
- Codegen: no changes.
- Compiler: Added `g_warn_level` global (declared in `include/util.h`, initialized to 0 in `src/util.c`); added `warn_level` local variable in `src/compiler.c` main(), parsed `-Wall` and `-Wextra` in argument loop with level assignment (`-Wall` sets level 1, `-Wextra` sets level 2; `-Wextra` implies `-Wall`), updated usage string.
- Wrapper (bin/cc99): Added `-Wall|-Wextra)` case in argument parser to forward both flags to `ccompiler`; updated usage block documentation.
- Tests: No new tests required. Full suite: 2072 portable (165 unit, 1286 valid, 261 invalid, 189 integration, 50 print-AST, 100 print-tokens, 21 print-asm) + 189 system-include + 2 optional-library pass.
- Docs: Updated `docs/outlines/checklist.md` to mark "Warning level support" complete and add 8 per-diagnostic sub-items for future stages.
- Notes: This stage mirrors Stage 142's approach for `-O0`/`-O1`: wire the plumbing first, implement actual diagnostic checks in follow-up stages. `-Wextra` is a strict superset of `-Wall` in terms of behaviors armed; all flag combinations produce identical NASM output.
