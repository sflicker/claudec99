# stage-14-06 Transcript: Char Array Initialization from String Literals

## Summary

Stage 14-06 teaches the compiler to initialize a local `char` array
from a string literal, both with an explicit size
(`char s[N] = "..."`) and with an inferred size
(`char s[] = "..."`). The literal's decoded payload bytes (from
Stage 14-05) plus a trailing NUL are copied into the array's stack
slot in declaration order; explicit-size arrays larger than the
literal are zero-filled in the remaining bytes. Inferred-size
arrays adopt `literal_byte_length + 1` as their length.

The grammar's array-size integer literal becomes optional, but is
only legal when paired with a string-literal initializer; the
parser enforces this and the three additional error categories
listed in the spec — "array too small for string literal
initializer", "string initializer only supported for char arrays",
and "omitted array size requires string literal initializer" — in
addition to keeping the existing "array size required unless
initialized from string literal" wording for the bare empty
brackets case. The literal is not pooled into `.rodata` for the
initializer path; codegen writes the bytes directly onto the stack.

## Plan

The plan paralleled the spec's section order: parser change to
accept the new declaration shape and emit the four spec errors;
codegen change to copy the literal payload, NUL, and zero fill into
the slot at declaration time; tests covering each spec example and
each error case; snapshot tests for AST and ASM output.

## Changes Made

### Step 1: Parser

- `src/parser.c`: in the array-suffix branch of the declaration
  handler:
  - Accept an empty `[]` and defer length resolution to the
    initializer.
  - Replace the unconditional
    `error: array initializers not supported` with a switch on the
    initializer form that emits the four spec-listed errors.
  - For empty `[]` with a string-literal initializer, infer
    `length = byte_length + 1`.
  - For explicit `[N]` with a string-literal initializer, enforce
    `N >= byte_length + 1`.
  - Build the `AST_STRING_LITERAL` child inline (mirroring
    `parse_primary`) so the initializer survives as a child of the
    `AST_DECLARATION` node.

### Step 2: Code Generator

- `src/codegen.c`: in `codegen_statement` / `AST_DECLARATION` for
  `TYPE_ARRAY`, capture the slot offset returned by
  `codegen_add_var` and, when a string-literal child is present,
  emit one `mov byte [rbp - (offset - i)], <byte>` per array byte.
  Bytes from the literal's payload come first, then the NUL
  terminator, then zero fill.

### Step 3: Tests

Valid runtime tests under `test/valid/`, one per spec example:

- `test_string_array_init_inferred_index_0__104.c`
- `test_string_array_init_inferred_index_1__105.c`
- `test_string_array_init_inferred_index_2__0.c`
- `test_string_array_init_explicit_5_index_4__0.c`
- `test_string_array_init_inferred_escape_n_index_1__10.c`
- `test_string_array_init_inferred_escape_null_index_2__66.c`

Invalid tests under `test/invalid/`:

- `test_invalid_48__array_too_small_for_string_literal_initializer.c`
- `test_invalid_49__string_initializer_only_supported_for_char_arrays.c`
- `test_invalid_50__array_size_required_unless_initialized_from_string_literal.c`
- `test_invalid_51__omitted_array_size_requires_string_literal_initializer.c`

Snapshot tests:

- `test/print_ast/test_stage_14_06_char_array_string_init.c` +
  `.expected`
- `test/print_asm/test_stage_14_06_char_array_string_init_explicit.c`
  + `.expected`
- `test/print_asm/test_stage_14_06_char_array_string_init_inferred.c`
  + `.expected`

### Step 4: Documentation

- `docs/grammar.md`: `<declaration>` updated so the array-size
  integer literal is optional, with a restriction note describing
  the string-literal-only inference rule and the char-only element
  rule.
- `README.md`: "Through stage" line, arrays bullet, "Not yet
  supported" section, and aggregate test counts updated.

## Final Results

Build clean. 402 / 402 tests pass (250 valid + 49 invalid + 23
print-AST + 73 print-tokens + 7 print-asm). No regressions.

## Session Flow

1. Read the Stage 14-06 spec and the SKILL / template / notes
   support files.
2. Reviewed the existing parser declaration path, codegen array
   handling, AST shape, and Stage 14-05 string-literal pipeline.
3. Produced a kickoff with delta from Stage 14-05, ambiguities
   flagged from the spec, and a step-by-step plan; saved under
   `docs/kickoffs/`.
4. Implemented the parser change, then the codegen change.
5. Built clean and authored runtime, invalid, AST, and ASM tests.
6. Captured AST and ASM snapshots from the compiler and saved
   them as `.expected` files.
7. Updated `docs/grammar.md` and `README.md`.
8. Ran the full suite to confirm 402/402 pass and recorded the
   milestone + transcript.

## Notes

- Spec annotation for `"hi"` (`105, 106, 0`) treated as a typo;
  conventional ASCII (`104, 105, 0`) used, consistent with the
  spec's later `s[0] // expect 104` test expectations and the
  spec's own codegen example.
- Spec valid test `char[5] = "hi";` (missing identifier) treated
  as a typo; implemented as `char s[5] = "hi";`.
- Spec inferred-size example `` ` char s[] = A\0B"; ` `` (missing
  opening quote) treated as `char s[] = "A\0B";` per surrounding
  text.
- The string-literal initializer is not pooled into `.rodata`; the
  bytes live only on the stack. This matches the spec's codegen
  example, which shows direct per-byte stores rather than a copy
  from a `.rodata` source.
