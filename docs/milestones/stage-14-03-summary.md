# Stage-14-03 Milestone: Emit String Literal to Static Data

## Completed

- Extended the code generator to handle `AST_STRING_LITERAL`. Each
  literal encountered during expression emission is appended to a
  per-translation-unit string pool and assigned a unique `Lstr<N>`
  label; the expression emits `lea rax, [rel Lstr<N>]` to load the
  literal's address into `rax`.
- After all functions are emitted, `codegen_translation_unit` writes
  a `section .rodata` with a `Lstr<N>:` label for each pooled
  literal, followed by a `db b1, b2, ..., 0` line that stores the
  payload bytes plus a NUL terminator. Empty literals collapse to
  `db 0`. The section is omitted entirely when no literals are
  collected, so existing programs are unaffected.
- The codegen result type is `TYPE_LONG` (an 8-byte
  address-as-integer view); pointer-typing of string literals is
  out of scope for this stage. This lets `int main() { return X; }`
  flow through the existing `emit_convert(LONG → INT)` path with no
  changes to the return-statement type-check.
- Added a new test class at `test/print_asm/`. Each test compiles a
  `.c` file in normal mode and diffs the produced `.asm` against a
  matching `.expected` file. Three tests cover the spec's examples
  (`return "hi"`, `return ""`, `return "one"`).
- Wired the new suite into `test/run_all_tests.sh` so `print_asm`
  runs as part of the project-wide aggregate.
- NASM syntax was retained throughout (`section .rodata`, `Lstr<N>:`
  without dot prefix, `db ...`); the spec's GAS-style example was
  translated rather than mixed in.

No tokenizer, parser, AST, or grammar changes — those landed in
stages 14-00 through 14-02.

## Files Changed

- `include/codegen.h` — added `MAX_STRING_LITERALS`, `string_pool[]`,
  and `string_pool_count` on `CodeGen`.
- `src/codegen.c` —
  - `codegen_init`: zero `string_pool_count`.
  - `codegen_expression`: new `AST_STRING_LITERAL` branch that
    pools the node and emits the address load.
  - `codegen_emit_string_pool` (new) + `codegen_translation_unit`:
    write `.rodata` after the function loop.
- `test/print_asm/test_stage_14_03_string_literal_basic.c` (+
  `.expected`).
- `test/print_asm/test_stage_14_03_string_literal_empty.c` (+
  `.expected`).
- `test/print_asm/test_stage_14_03_string_literal_one.c` (+
  `.expected`).
- `test/print_asm/run_tests.sh` — new suite runner.
- `test/run_all_tests.sh` — added `print_asm` to the suites list.

## Test Results

- Valid:        232 / 232 pass.
- Invalid:       44 /  44 pass.
- Print-AST:     21 /  21 pass.
- Print-Tokens:  72 /  72 pass.
- Print-Asm:      3 /   3 pass (new).
- Aggregate:    372 / 372 pass. No regressions.
