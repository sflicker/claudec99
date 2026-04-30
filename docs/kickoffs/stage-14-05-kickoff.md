# Stage-14-05 Kickoff

## Spec
`docs/stages/ClaudeC99-spec-stage-14-05-string-escape-sequences.md`

## Stage Goal
Allow string literals to contain six backslash escape sequences whose
bytes are decoded by the lexer:

- `\n` → 10
- `\t` → 9
- `\r` → 13
- `\\` → 92
- `\"` → 34
- `\0` → 0

The literal's logical length is the byte count after decoding,
excluding the implicit terminating NUL added by codegen. Codegen
emits the decoded bytes via `db`. Any other `\<char>` is rejected as
"invalid escape sequence". Token and AST pretty-printers re-escape
the decoded bytes so output remains stable and line-oriented.

## Delta from Stage 14-04
Stage 14-04 left the lexer rejecting every backslash inside a string
literal. Stage 14-05 replaces that rejection with a small decoder for
the six listed sequences, propagates the decoded byte buffer (which
may contain embedded NULs for `\0`) through the token, AST, and
codegen pipelines, and rejects all other escapes.

## Ambiguities Flagged
- The spec example for the AST pretty-printer (`StringLiteral: "A\n"`)
  omits the `(length N)` suffix added in Stage 14-02. Reading the
  text "should *also* escape" as additive, the existing `(length N)`
  suffix is preserved and only the value rendering is changed.
- The phrase "escape printable characters" in the AST section is
  read as a typo for "escape non-printable characters", consistent
  with the stated goal "so expected output remains line-oriented".
- One spec invalid test source is missing its closing `}`. Treated
  as a typo and corrected in the corresponding test file.

## Planned Changes

### Tokenizer (`src/lexer.c`)
- Replace the unconditional rejection of `\\` inside a string literal
  with a small switch that decodes `\n`, `\t`, `\r`, `\\`, `\"`, `\0`.
- Any other `\<char>` raises
  `error: invalid escape sequence in string literal`.
- The decoded byte is stored in `token.value`; the existing
  `token.length` counts decoded bytes, so embedded NULs are
  preserved.

### Parser (`src/parser.c`)
- In `parse_primary`, for `AST_STRING_LITERAL`, replace
  `ast_new(..., token.value)` (which uses `strncpy`, truncating at
  the first embedded NUL) with constructing the node and
  `memcpy`'ing exactly `token.length` bytes into `node->value`.
- Set the new `node->byte_length = token.length`.

### AST (`include/ast.h`, `src/ast.c`)
- Add `int byte_length` to `ASTNode`. Default-initialized to 0 by
  `calloc` in `ast_new`. Used only by `AST_STRING_LITERAL`.

### AST Pretty Printer (`src/ast_pretty_printer.c`)
- Replace direct `%s` rendering of the literal payload with a helper
  that walks `byte_length` bytes and re-escapes back to source form.
  Existing `(length N)` suffix is retained.

### Token Printer (`src/compiler.c`)
- In `print_token_row`, for `TOKEN_STRING_LITERAL` build an escaped
  representation from the first `length` bytes before applying the
  existing 15/18-char width logic so embedded NULs and escape bytes
  print as their source spellings.

### Code Generator (`src/codegen.c`)
- `codegen_emit_string_pool` switches from `strlen(s->value)` to
  `s->byte_length` so embedded NUL bytes are no longer silently
  truncated.

### Tests
Valid (`test/valid/`), one per spec example:
- `test_string_literal_escape_n_newline__10.c`
- `test_string_literal_escape_r__13.c`
- `test_string_literal_escape_backslash__92.c`
- `test_string_literal_escape_quote__34.c`
- `test_string_literal_escape_null__0.c`
- `test_string_literal_escape_null_after__66.c`

Invalid (`test/invalid/`):
- `test_invalid_47__invalid_escape_sequence.c`

Token, AST, and ASM snapshots:
- `test/print_tokens/test_token_string_literal_escapes.c` + `.expected`
- `test/print_ast/test_stage_14_05_string_literal_escapes.c` + `.expected`
- `test/print_asm/test_stage_14_05_string_literal_escape_n.c` + `.expected`
- `test/print_asm/test_stage_14_05_string_literal_escape_null.c` + `.expected`

### Documentation
- `docs/grammar.md`: extend `<string_literal>` to mention the
  supported escape set.
- Save milestone + transcript at end of stage.

### Commit
Single commit at the end of the stage.
