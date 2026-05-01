# Milestone Summary

## Stage-14-08: Additional invalid tests for char-array support — Complete

- Added four new files to `test/invalid/` covering the spec
  cases that were not yet exercised:
  - `test_invalid_52__invalid_escape_sequence.c` — rejects `"\q"`
    (single-character unsupported escape, complementing
    `test_invalid_47` which exercises the hex form `\x41`).
  - `test_invalid_53__assigning_pointer_to_non_pointer.c` —
    rejects `int x = "hello";` (string literal assigned to a
    scalar int).
  - `test_invalid_54__assigning_pointer_to_non_pointer.c` —
    rejects `char c = "x";` (string literal assigned to a scalar
    char).
  - `test_invalid_55__string_initializer_only_supported_for_char_arrays.c`
    — rejects `int s[3] = "hi";` (non-char array with explicit
    size, complementing `test_invalid_49` which exercises the
    inferred form `int s[] = "hi";`).
- Existing diagnostics emitted by the compiler proved stable enough
  to satisfy the filename-based expected-fragment matching; no
  changes to lexer, parser, AST, or codegen were required, in line
  with the spec's no-compiler-changes constraint.
- `README.md`: bumped to "Through stage 14-08", and the test-count
  line updated from 405 to 409 (252 valid + 53 invalid + 23
  print-AST + 73 print-tokens + 8 print-asm).
- Full suite: 409/409 pass; no regressions.
