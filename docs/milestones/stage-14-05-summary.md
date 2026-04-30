# Milestone Summary

## Stage-14-05: String Literal Escape Sequences — Complete

The lexer now decodes the six common backslash escape sequences
(`\n`, `\t`, `\r`, `\\`, `\"`, `\0`) inside string literals into
their actual byte values. The decoded payload — which may contain
embedded NUL bytes from `\0` — is carried through the token, AST,
and codegen pipelines. `--print-tokens` and `--print-ast` re-escape
those bytes back to source form so output stays line-oriented and
diff-stable. Codegen emits the decoded byte sequence via `db`, with
a trailing 0 added by the codegen pool emitter. Any other backslash
sequence (e.g. `\x41`, `\101`) is rejected with `error: invalid
escape sequence in string literal`.

## Behavior gained
- `"A\n"` → bytes 65, 10 (literal length 2)
- `"A\rB"` → bytes 65, 13, 66 (literal length 3)
- `"A\\B"` → bytes 65, 92, 66 (literal length 3)
- `"A\"B"` → bytes 65, 34, 66 (literal length 3)
- `"A\0B"` → bytes 65, 0, 66 (literal length 3)
- `"\x41"` → rejected as `invalid escape sequence`

## Code changes
- `include/ast.h`: added `int byte_length` to `ASTNode` so the
  decoded byte count survives the array-to-pointer decay performed
  during expression codegen (where `full_type` is rewritten to
  `char *`).
- `src/lexer.c`: replaced the unconditional rejection of `\\` inside
  a string literal with a small switch decoding the six supported
  escapes; all other escape sequences are rejected.
- `src/parser.c`: switched the `AST_STRING_LITERAL` construction
  from `ast_new(..., token.value)` (which uses `strncpy` and
  truncates at the first NUL) to `memcpy` of `token.length` bytes
  and stamps `node->byte_length`.
- `src/ast_pretty_printer.c`: `AST_STRING_LITERAL` rendering walks
  `byte_length` bytes and re-escapes back to source form.
- `src/compiler.c`: `print_token_row` handles `TOKEN_STRING_LITERAL`
  specially, building the escaped form before applying the existing
  15/18-char width logic.
- `src/codegen.c`: `codegen_emit_string_pool` now reads the byte
  count from `s->byte_length` instead of `strlen(s->value)`.

## Test changes
- `test/valid/`: added six runtime tests, one per spec example.
- `test/invalid/`: added
  `test_invalid_47__invalid_escape_sequence.c`; removed the now
  obsolete `test_invalid_45__escape_sequences_not_supported.c`,
  whose behavior is reversed by this stage.
- `test/print_tokens/`:
  `test_token_string_literal_escapes.c` + `.expected`.
- `test/print_ast/`:
  `test_stage_14_05_string_literal_escapes.c` + `.expected`.
- `test/print_asm/`:
  `test_stage_14_05_string_literal_escape_n.c` + `.expected` and
  `test_stage_14_05_string_literal_escape_null.c` + `.expected`.

## Documentation
- `docs/grammar.md`: extended `<string_literal>` with the supported
  `<escape_sequence>` set.

## Build & tests
Build clean. 389 / 389 tests pass (244 valid + 45 invalid + 22
print-AST + 73 print-tokens + 5 print-asm). No regressions.
