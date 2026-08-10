# Stage 177 Kickoff: Assembler — Lexer and Parser Infrastructure

## Spec Summary

Stage 177 begins a new, standalone NASM-subset assembler tool called `casm` that will eventually replace the external `nasm` dependency. This stage adds **lexer and parser infrastructure only** — no instruction encoding, no symbol table, no ELF output.

`casm` is a new independent executable that does not touch existing ccompiler source. It reads a `.asm` file in the exact NASM-syntax subset that `src/codegen.c` emits and parses it into a flat array of `AsmLine` records. With `--dump` it prints normalized output; otherwise it reports a line count and exits 0. This proves the front-end is complete and correct against real compiler output before encoding begins.

The lexer and parser are scoped against the full self-hosted build corpus (~216K lines across 20 `.asm` files under `build/c1_objs/` and `build/c2_objs/`). They must parse 100% of that corpus without error.

## Implementation Plan

### Files to Create

1. **`include/asm_lexer.h`** — Token types, `AsmLexer`, `AsmToken` structs, and standalone helper function declarations (`asm_read_file`, `asm_strdup`).

2. **`src/asm_lexer.c`** — Lexer implementation. Recognizes identifiers (labels, mnemonics, directives, registers, symbols — including `.` and `_`; also hyphen `-` in identifier characters), decimal and `0x`-hex integers, `[ ] + - , :` operators, newlines as explicit line terminators, and `;`-comments (to end of line). Whitespace consumed silently.

3. **`include/asm_parser.h`** — `AsmOperand` enum and struct (four memory forms: `[reg]`, `[rbp±disp]`, `[rsp+disp]`, `[rel symbol]`), `AsmLine` enum and struct (LABEL/DIRECTIVE/INSTRUCTION kinds), and parser entry point `asm_parser_parse()`.

4. **`src/asm_parser.c`** — Recursive-descent parser. Register table, directive classifier, operand parsers (full operand form, data values for `db`/`dd`/`dq`, bare args for other directives), line parser producing flat `Vec<AsmLine>`. Includes `asm_line_free()` and `asm_dump_line()` for cleanup and debug output.

5. **`src/casm.c`** — CLI driver: `casm <file.asm> [--dump]`. Parses the file and either dumps parsed lines or reports line count.

6. **`CMakeLists.txt`** — Add `casm` executable target alongside `ccompiler`. Links `src/vec.c` but not `src/util.c` (keep standalone).

7. **`test/unit/test_asm_lexer.c`** — 28 assertions. Hand-written `.asm` snippet tests: instruction line, label line, hex immediate, comments, etc.

8. **`test/unit/test_asm_parser.c`** — 56 assertions. Parser tests: bare label, directives, data directives, all four memory operand forms, signed/hex immediates, zero and multi-operand instructions.

9. **`test/asm/run_casm_smoke.sh`** — Corpus smoke test: glob `build/**/*.asm`, run `casm` on each, report pass/fail counts.

### Implementation Order

1. `include/asm_lexer.h` and `src/asm_lexer.c` — Lexer with token types and helper functions.
2. `include/asm_parser.h` and `src/asm_parser.c` — Operand and line models, recursive-descent parser.
3. `src/casm.c` — CLI driver.
4. `CMakeLists.txt` — Add the `casm` target.
5. Build and smoke test by hand.
6. Unit tests: `test_asm_lexer.c` and `test_asm_parser.c`, registered in `test/unit/run_tests.sh`.
7. Corpus smoke test: `test/asm/run_casm_smoke.sh` after a self-hosted build.
8. `src/version.c` — Bump `VERSION_STAGE` to `"01770000"`.
9. `docs/outlines/checklist.md` — Mark lexer/parser/operand and Milestone 1 items complete under "Assembler" section; add `## Stage 177` section.

## Known Ambiguities and Pre-Implementation Notes

### Checklist Structural Difference (Already Resolved in Spec)

The pre-existing `docs/outlines/checklist.md` anticipated a separate `include/asm_operand.h`/`src/asm_operand.c` module for the operand model. The stage-177 spec places `AsmOperand` directly in `asm_parser.h` for simplicity — a deliberate spec-directed consolidation. No separate file needed.

### Expected Corpus Gaps (Discovered During Implementation)

The spec states the design was "measured against the full self-hosted build output," but real-world testing will likely reveal three implementation adjustments:

1. **Hyphenated identifiers**: Directives like `section .note.GNU-stack noalloc noexec nowrite progbits` have hyphens inside identifiers. The spec's `is_ident_char` logic may need `-` added to handle `GNU-stack` as a single token (while preserving `[rbp - 16]` displacement parsing via whitespace handling).

2. **Label-attached directives**: Real codegen output attaches data directives directly to labels on the same line — e.g. `Lstr0: db 72, 101, 108, 108, 111, 0`. The spec's line parser may need to detect "label: directive ..." and emit two `AsmLine` records.

3. **Missing directive names**: `resd`, `resq`, `resw` (reserve 4/8/2-byte storage, analogous to `resb`) may be present in the corpus but absent from the spec's known-directive list.

All three are expected to be resolved by adding permissive support for these edge cases — the fix is to make the parser more liberal in what it accepts, not to change the corpus test expectations.

## Test Plan

- **Unit tests**: Hand-written snippets covering lexer token stream and parser output for all operand forms and line types.
- **Corpus smoke test**: Parse every `.asm` file in `build/` (after self-hosted build), verify 0 exits for all compiler-generated output.
- **No self-host run required** — `casm` is not part of the C0→C1→C2 chain yet; `build.sh --mode=self-host` is unaffected.

## Bootstrap Notes

`casm` is built only with the system compiler via CMake. The new files are **not** added to `build.sh`'s source list or to `ccompiler`'s CMake sources. `nasm` remains the only assembler actually used until `casm` reaches ELF parity (later stages).
