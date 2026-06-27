# Milestone Summary

## Stage 169: Debug Information (DWARF) - Complete

stage-169 ships DWARF debug information support via the `-g` flag, enabling GDB to set breakpoints by source line and step through C code.

- Tokenizer: no changes.
- Grammar/Parser: no changes.
- AST/Semantics: no changes.
- Codegen: Added `emit_debug` (int), `debug_last_file` (const char *), `debug_last_line` (int) to `CodeGen` struct; new `emit_debug_line()` static helper guards on `emit_debug`, skips invalid positions, deduplicates by file+line; called before function labels in `codegen_function()` and after `cg_mark()` in `codegen_statement()`; fields zero-initialized in `codegen_init()`.
- Compiler: Added `emit_debug` local variable, `-g` branch in argument parser, `emit_debug` parameter to `compile_one_file()`, set `cg.emit_debug` before codegen, updated usage string.
- Wrapper (bin/cc99): Added `-g` case, post-loop debug detection, pass `-g -F dwarf` to NASM when debug enabled.
- Tests: 1 new integration test (`test_dwarf_debug`, compiled with `-g`, exits 42). Full suite: 2072 portable (165 unit, 1286 valid, 261 invalid, 189 integration, 50 print-AST, 100 print-tokens, 21 print-asm) + 189 system-include + 2 optional-library pass.
- Docs: README updated with Stage 169 description.
- Notes: When `-g` is passed, compiler emits NASM `%line N+0 "file.c"` directives before function labels and statement dispatches. NASM (assembled with `-g -F dwarf`) produces `.debug_line`, `.debug_abbrev`, `.debug_info` ELF sections. GDB shows source file and line numbers at breakpoints.
