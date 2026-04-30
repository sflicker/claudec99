# Stage-14-03 Kickoff: Emit String Literal to Static Data

## Spec Summary

Stage 14-03 wires `AST_STRING_LITERAL` (added in stage 14-02) into
codegen. The compiler now:

  - Assigns each string-literal expression a unique label.
  - Emits the literal's bytes (plus a trailing NUL) into a static
    read-only data section.
  - Emits the literal's address into `rax` whenever it appears in
    expression context (`lea rax, [rel <label>]`).

A new test class lives in `test/print_asm/`: each test compiles a
`.c` to a `.asm` and diffs against a `.expected` file. Three tests
are required (`return "hi"`, `return ""`, `return "one"`) plus a
runner that the top-level `run_all_tests.sh` will invoke.

Out of scope: assigning to `char *`, indexing, escape sequences,
char-array initialization from string literals, adjacent-literal
concatenation, libc calls.

## What Must Change vs. Stage 14-02

  - `include/codegen.h` — new string-pool state on `CodeGen`.
  - `src/codegen.c` —
      - Handle `AST_STRING_LITERAL` in `codegen_expression`: register
        the literal in the pool and emit its address.
      - Append the `.rodata` section in `codegen_translation_unit`
        after all functions are emitted.
  - `test/print_asm/` — new directory with three `.c` tests, three
    `.expected` files, and `run_tests.sh`.
  - `test/run_all_tests.sh` — add `print_asm` to the suites list.
  - No tokenizer, parser, AST, or grammar changes — those landed in
    stages 14-00 through 14-02.

## Spec Issues Flagged

  1. **GAS vs. NASM syntax.** The spec's example assembly uses GAS
     directives (`.section .rodata`, `.byte 104, 105, 0`, `.Lstr0:`)
     but the project emits NASM (`section .text`, `db ...`). NASM is
     locked in by the test runner (`nasm -f elf64 ...`). Mapping:

     | Spec (GAS)                  | This project (NASM)        |
     |-----------------------------|----------------------------|
     | `.section .rodata`          | `section .rodata`          |
     | `.Lstr0:`                   | `Lstr0:` (no dot)          |
     | `.byte 104, 105, 0`         | `db 104, 105, 0`           |
     | `lea rax, [rel .Lstr0]`    | `lea rax, [rel Lstr0]`     |

     A leading dot in NASM marks a label local to the most recent
     non-local label, which is the wrong scope for a `.rodata` data
     symbol; using `Lstr<N>` (no dot) keeps it file-scope.

  2. **Return-type interaction.** The current type checker rejects
     `int main() { return X; }` when `X->result_type == TYPE_POINTER`.
     The spec's first test must compile, but the spec is permissive
     about expression-level type — its type rule says the literal is
     "logically `char[N]`" and "may be treated as an address". I'll
     classify the codegen result as `TYPE_LONG` (an 8-byte
     address-as-integer). The existing `emit_convert(LONG → INT)` is
     a no-op (since `eax` already holds the low 32 bits of `rax`),
     so `int main` keeps producing the expected exit value with no
     changes to the return-statement type-check. Extending real
     pointer typing of string literals is explicitly out of scope.

## Decisions

  1. **Label naming.** `Lstr<N>` (no dot prefix), counter unique per
     translation unit, allocated as each `AST_STRING_LITERAL` is
     encountered during expression emission.
  2. **Pool storage.** A fixed-capacity array of `ASTNode *` on
     `CodeGen` (the AST outlives codegen, so pointers are stable).
     `MAX_STRING_LITERALS = 256`. Each entry's payload bytes and
     length are recovered from `node->value` and
     `node->full_type->length - 1` (the `full_type` was set to
     `char[N+1]` by stage 14-02).
  3. **`db` form.** Numeric byte sequence (`db 104, 105, 0`) rather
     than NASM's string form (`db "hi", 0`) — matches the spec
     example, sidesteps quoting, and makes the empty case
     (`db 0`) trivial.
  4. **Section ordering.** `.text` first (functions), then `.rodata`
     once at the end if any literals were collected. Matches the
     spec's recommendation and avoids interleaving sections.
  5. **Type of the result.** `TYPE_LONG` (see Spec Issue §2).

## Planned Changes

  1. **Tokenizer** — none.
  2. **Parser** — none.
  3. **AST** — none.
  4. **Code generator** —
      - `include/codegen.h`: add `MAX_STRING_LITERALS`, a
        `string_pool[]` of `ASTNode *`, and a `string_pool_count`
        on `CodeGen`.
      - `src/codegen.c`:
          - `codegen_init`: zero the new counter.
          - `codegen_expression`: handle `AST_STRING_LITERAL` —
            record the node, emit
            `lea rax, [rel Lstr<idx>]`, set
            `result_type = TYPE_LONG`.
          - `codegen_translation_unit`: after the function loop,
            if the pool is non-empty, emit `section .rodata`, then
            for each entry emit `Lstr<i>:` and a `db b1, b2, ..., 0`
            line built from the payload bytes.
  5. **Tests** — create `test/print_asm/`:
      - `test_stage_14_03_string_literal_basic.c` (+`.expected`) →
        `return "hi";`.
      - `test_stage_14_03_string_literal_empty.c` (+`.expected`) →
        `return "";`.
      - `test_stage_14_03_string_literal_one.c` (+`.expected`) →
        `return "one";`.
      - `run_tests.sh` mirroring the existing print-suite runner
        pattern (compile, diff vs. `.expected`).
      - Add `print_asm` to the suites in `test/run_all_tests.sh`.
  6. **Documentation** — none required (grammar unchanged).
  7. **Commit** — single commit when the aggregate suite is green.
