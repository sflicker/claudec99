# Milestone Summary

## Stage 84: Standard Streams - Complete

stage-84 ships standard C stream pointers (`stdin`, `stdout`, `stderr`) as extern FILE* declarations in the controlled stdio.h stub header.

- **Tokenizer**: No changes.
- **Grammar/Parser**: No changes.
- **AST/Semantics**: No changes.
- **Codegen**: Enhanced to handle `extern`-declared object variables. Global variables now track an `is_extern` flag; extern globals are registered in the globals table but skipped in BSS/data emission. `codegen_emit_externs` now emits NASM `extern` directives for is_extern globals so the linker can resolve them.
- **Tests**: 1 new test added (`test_stdio_streams__1.c`). Full suite 1260/1260 pass.
- **Docs**: Updated README to list stream pointers in stdio.h support summary and updated test counts.
- **Notes**: None.
