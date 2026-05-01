# Milestone Summary

## Stage-14-07-01: Optional stdout comparison in valid tests — Complete

- `test/valid/run_tests.sh`: each binary's stdout is now captured
  to a per-test temp file. After the exit-code check, if a sibling
  `<name>.expected` file exists in the source directory, the
  captured stdout is compared against it byte-for-byte; mismatch is
  reported as `FAIL (output mismatch)` with a unified diff. Tests
  without a `.expected` file continue to be validated by exit code
  alone.
- `test/valid/run_test.sh`: single-test driver gained the same
  optional `.expected` comparison.
- New fixtures back-filling the existing puts tests:
  - `test/valid/test_libc_puts_hello__0.expected`
  - `test/valid/test_libc_puts_two_calls__0.expected`
- `README.md`: `valid` row in the suites table updated to describe
  the optional stdout check.
- No changes to the compiler (lexer, parser, AST, codegen).
- Result: 405/405 tests pass (252 valid + 49 invalid + 23
  print-AST + 73 print-tokens + 8 print-asm). The two puts tests
  now additionally verify that `puts` writes the expected bytes to
  stdout, not just that the program exits with the expected code.
