# stage-14-03 Transcript: Emit String Literal to Static Data

## Summary

Stage 14-03 adds code generation for string literals. The parser
and AST landed in earlier stages of series 14; this stage wires
`AST_STRING_LITERAL` into `codegen_expression` and emits a static
read-only data section after every function.

Each string-literal expression registers its `ASTNode *` in a
per-translation-unit pool and is assigned the next `Lstr<N>` label.
The expression emits `lea rax, [rel Lstr<N>]` to materialize the
address. When the function loop completes, `codegen_translation_unit`
writes a `section .rodata` followed by a `Lstr<N>:` label and a
`db b1, b2, ..., 0` line for each pooled literal; the section is
omitted entirely when no literals were collected.

Two semantic constraints keep the implementation minimal. (1) The
spec's example assembly used GAS directives but the project emits
NASM, so labels are written without a dot prefix and bytes use `db`.
(2) The codegen result type is `TYPE_LONG` so the spec's first test
(`int main() { return "hi"; }`) flows through the existing
return-statement type-check unchanged. Pointer-typing of string
literals, indexing, `char *` assignment, and escape sequences remain
out of scope.

## Plan

After reading the spec, the planning phase decided to:

- Translate the spec's GAS syntax to NASM throughout (`section
  .rodata`, `Lstr<N>:` without a leading dot, `db` byte sequences,
  `lea rax, [rel Lstr<N>]`).
- Type the codegen result as `TYPE_LONG` to avoid touching the
  return-statement pointer/integer type-check while still letting
  `int main` return the literal's address truncated through `eax`.
- Pool string literals as `ASTNode *` on `CodeGen` (the AST outlives
  codegen) and emit `.rodata` once at the end of the translation
  unit, matching the spec's recommendation.
- Add a new `test/print_asm` suite mirroring the existing print-suite
  pattern: compile, diff against `.expected`.

## Changes Made

### Step 1: Code Generator State

- Added `MAX_STRING_LITERALS` (256) to `include/codegen.h`.
- Added `ASTNode *string_pool[MAX_STRING_LITERALS]` and
  `int string_pool_count` to the `CodeGen` struct.
- Initialized `string_pool_count = 0` in `codegen_init`.

### Step 2: Code Generator — Expression

- Added an `AST_STRING_LITERAL` branch to `codegen_expression` in
  `src/codegen.c`. The branch:
  - Rejects pools that have reached `MAX_STRING_LITERALS`.
  - Stores the AST node pointer in `string_pool[idx]` and
    increments `string_pool_count`.
  - Emits `lea rax, [rel Lstr<idx>]`.
  - Sets `node->result_type = TYPE_LONG`.

### Step 3: Code Generator — Translation Unit

- Added `codegen_emit_string_pool` to walk the pool once and emit
  `section .rodata`, then for each entry a `Lstr<i>:` label
  followed by a `db b1, b2, ..., 0` line built from the payload
  bytes. Empty literals reduce to `db 0`. The whole section is
  skipped when the pool is empty.
- `codegen_translation_unit` now calls
  `codegen_emit_string_pool(cg)` after the function loop.

### Step 4: Tests

- Created `test/print_asm/` with three tests, each paired with a
  matching `.expected` file:
  - `test_stage_14_03_string_literal_basic.c` — `return "hi";`.
  - `test_stage_14_03_string_literal_empty.c` — `return "";`.
  - `test_stage_14_03_string_literal_one.c` — `return "one";`.
- Added `test/print_asm/run_tests.sh`. The runner compiles each
  test from inside a per-suite work directory (the compiler writes
  the `.asm` into the current working directory using only the
  source basename) and diffs the produced `.asm` against
  `.expected`.
- Updated `test/run_all_tests.sh` to include `print_asm` in the
  suites list so the new suite participates in the aggregate.

## Final Results

Build clean. All 372 tests pass (232 valid + 44 invalid + 21
print-AST + 72 print-tokens + 3 print-asm). Three new tests added;
no regressions.

## Notes

- The spec's example assembly used GAS directives (`.section
  .rodata`, `.byte`, `.Lstr0`). NASM was kept because the project's
  test harness pipes through `nasm -f elf64 ...`. The `.Lstr0`
  GAS-local-label spelling would behave incorrectly in NASM (a
  leading dot makes the label local to the previous non-local
  symbol, the wrong scope for a `.rodata` data label), so the dot
  was dropped.
- The codegen result type was deliberately kept as `TYPE_LONG`. A
  future stage that introduces `char *` typing of string literals
  will need to revisit `expr_result_type` and the
  return/declaration/assignment paths together.

## Session Flow

1. Read the spec, the prior 14-series milestones, and the existing
   codegen, parser, and pretty-printer.
2. Confirmed the GAS-vs-NASM and return-type ambiguities and chose
   NASM + `TYPE_LONG` as stage-limited resolutions.
3. Produced a kickoff covering decisions and planned changes;
   paused for confirmation.
4. Added the string-pool state and `codegen_init` reset.
5. Implemented the `AST_STRING_LITERAL` branch in
   `codegen_expression` and the new `codegen_emit_string_pool`
   helper, called from `codegen_translation_unit`.
6. Built the compiler and dumped output for each spec example
   program; confirmed the `.asm` assembled cleanly with NASM and
   linked into runnable binaries.
7. Authored the three `.c` / `.expected` pairs and the
   `test/print_asm/run_tests.sh` runner; wired the suite into
   `test/run_all_tests.sh`.
8. Fixed the runner's `.asm` path expectation (the compiler writes
   the file into `$PWD` using only the basename) by running the
   compiler from inside the work directory.
9. Ran the aggregate suite — 372/372 pass.
10. Wrote the milestone summary and this transcript.
