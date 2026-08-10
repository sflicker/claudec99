# Milestone Summary

## Stage 177: Assembler — Lexer and Parser Infrastructure - Complete

Stage 177 ships a standalone NASM-subset assembler tool (`casm`) with a complete lexer and parser front-end for the exact assembly syntax that `src/codegen.c` emits, reading `.asm` files into a flat array of `AsmLine` records.

- **Lexer**: Recognizes identifiers (including `.`, `_`, and `-` for hyphenated names), decimal and `0x`-hex integers, bracket and operator tokens, newlines, and `;`-comments. Standalone file reading and string duplication helpers (`asm_strdup`, `asm_read_file`). ~200 lines of implementation.

- **Parser**: Recursive-descent parser producing `Vec<AsmLine>` with three line kinds (LABEL/DIRECTIVE/INSTRUCTION) and four memory operand forms (`[reg]`, `[rbp±disp]`, `[rsp+disp]`, `[rel symbol]`). Directive argument parsing: comma-separated for `db`/`dd`/`dq`, space-separated for all others. ~500 lines of implementation.

- **CLI**: `casm <file.asm> [--dump]` prints normalized line dump or reports line count. Integrated into CMake as a second executable target alongside `ccompiler`, copied to `bin/casm` post-build.

- **Corpus fixes** (discovered during testing): Three spec refinements were needed to parse 100% of the genuine self-hosted compiler output (~216K lines, 20 files):
  1. Added `-` to `is_ident_char()` to accept hyphenated identifiers like `GNU-stack` in section attributes.
  2. Parser detects label-attached directives (e.g. `Lstr0: db 72, ...`) and emits two `AsmLine` records (LABEL, then DIRECTIVE) sharing the same `line_no`.
  3. Added `resd`, `resq`, `resw` to the known-directive list (analogous to `resb`, for 4/8/2-byte storage reservation).

- **Tests**: 
  - Unit tests: 28 lexer assertions + 56 parser assertions (2 new test files registered in `test/unit/run_tests.sh`).
  - Full portable suite: 2161 passed (was 2077; +84 from new unit tests), 0 failed.
  - Corpus smoke test: 317 passed, 3 failed (known out-of-scope failures in unrelated `test_tmp_*` directories; 100% pass on genuine compiler output).
  - No self-host run performed (not required per spec — `casm` is not part of C0→C1→C2 chain yet).

- **Docs**: Checklist Assembler section updated (lexer/parser/operand-model items and Milestone 1 marked complete); new `## Stage 177` section added to checklist; `VERSION_STAGE` bumped to `"01770000"`. `casm` is a new, standalone tool not yet wired into the build/link pipeline — `nasm` remains the active assembler.

- **Notes**: The pre-existing checklist anticipated a separate `asm_operand.h` module; the spec consolidates `AsmOperand` into `asm_parser.h` (no separate file needed). Three integration test failures are explicitly out-of-scope (NASM preprocessor directives, floating-point operands, RIP-relative displacements — never emitted by the compiler itself). The spec's "Bootstrap caveats" section explicitly states no self-host run is required; stage-177 source files are not added to `build.sh` or `ccompiler`'s CMake sources.
