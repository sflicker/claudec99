# Milestone Summary

## Stage-14-06: Char Array Initialization from String Literals — Complete

Local `char` arrays may now be initialized from a string literal,
both with an explicit size (`char s[N] = "..."`) and with an
inferred size (`char s[] = "..."`). The literal's decoded payload
plus a trailing NUL is copied into the stack slot at declaration
time; for explicit-size arrays larger than the literal, the
remaining bytes are zero-filled. Inferred-size arrays adopt
`literal_byte_length + 1` as their length.

## Behavior gained
- `char s[] = "hi";` — array length 3, bytes 104, 105, 0.
- `char s[5] = "hi";` — bytes 104, 105, 0, 0, 0.
- `char s[] = "A\n";` — array length 3, bytes 65, 10, 0.
- `char s[] = "A\0B";` — array length 4, bytes 65, 0, 66, 0
  (embedded NUL preserved from the Stage 14-05 escape decoder).

## New rejections
- `char s[2] = "hi";` → `array too small for string literal initializer`.
- `int s[] = "hi";` → `string initializer only supported for char arrays`.
- `char s[];` → `array size required unless initialized from string literal`.
- `char s[] = 123;` → `omitted array size requires string literal initializer`.

## Code changes
- `src/parser.c`: array-suffix branch in the declaration handler
  now accepts an empty `[]`, accepts a string-literal initializer
  for `char` arrays, infers the length from the literal when the
  size is omitted, and validates explicit-size capacity. The four
  spec-listed error categories are emitted with the wording from
  the spec.
- `src/codegen.c`: `AST_DECLARATION` for `TYPE_ARRAY` now reads the
  optional `AST_STRING_LITERAL` child and emits per-byte stack
  stores covering the entire array — payload bytes, trailing NUL,
  then zero fill out to the declared size. The literal is not
  pooled into `.rodata` for the initializer path; the bytes live
  only on the stack.

## Test changes
- `test/valid/`: six runtime tests, one per spec example.
- `test/invalid/`: four invalid tests, one per spec error case.
- `test/print_ast/`: `test_stage_14_06_char_array_string_init.c`
  + `.expected`.
- `test/print_asm/`:
  `test_stage_14_06_char_array_string_init_explicit.c` + `.expected`
  and `test_stage_14_06_char_array_string_init_inferred.c` +
  `.expected`.

## Documentation
- `docs/grammar.md`: `<declaration>` updated to make the array-size
  integer literal optional, with a note describing the
  string-literal restriction.
- `README.md`: "Through stage" line, arrays bullet, "Not yet
  supported" section, and aggregate test counts updated to reflect
  Stage 14-06 completion.

## Build & tests
Build clean. 402 / 402 tests pass (250 valid + 49 invalid + 23
print-AST + 73 print-tokens + 7 print-asm). No regressions.
