# stage-14-05 Transcript: String Literal Escape Sequences

## Summary

Stage 14-05 teaches the lexer to decode the six common backslash
escape sequences inside string literals (`\n`, `\t`, `\r`, `\\`,
`\"`, `\0`) into their byte values, and propagates the decoded
payload — including embedded NUL bytes — through the token, AST,
and codegen pipelines. Any other backslash sequence (e.g. `\x41`,
`\101`) is rejected as `invalid escape sequence`.

The literal's logical length is the post-decode byte count,
recorded on the lexer's `Token.length` and copied onto a new
`ASTNode.byte_length` field so it survives the Stage 14-04
array-to-pointer decay during expression codegen. `--print-tokens`
and `--print-ast` re-escape the decoded bytes back to source form so
their output remains line-oriented and stable for diff testing. The
existing `.rodata` emitter now reads the byte count from
`byte_length` instead of `strlen`, so `"A\0B"` correctly emits
`db 65, 0, 66, 0`.

Hex (`\x..`), octal (`\0..` beyond a single zero), Unicode escapes,
adjacent string-literal concatenation, character literals, and
char-array initialization from string literals remain out of scope.

## Plan

The plan paralleled the spec's section order: lexer decode →
in-flight AST length carrier → parser memcpy → pretty printers →
codegen pool. The token-printer change was identified during code
review as a side requirement of allowing embedded NULs through
`Token.value`.

## Changes Made

### Step 1: AST

- Added `int byte_length` to `ASTNode` in `include/ast.h`. Used by
  `AST_STRING_LITERAL` to carry the decoded byte count past the
  point where `full_type` is rewritten to `char *`.

### Step 2: Tokenizer

- In `src/lexer.c`, replaced the unconditional `\\`-rejection branch
  with a switch decoding `\n` → 10, `\t` → 9, `\r` → 13, `\\` → 92,
  `\"` → 34, `\0` → 0.
- Any other `\<char>` raises
  `error: invalid escape sequence in string literal`.
- Token storage already supported a length-bearing payload via
  `Token.length`; the existing 255-byte cap and unterminated /
  newline-in-literal checks were preserved.

### Step 3: Parser

- In `src/parser.c`, the `AST_STRING_LITERAL` builder now calls
  `ast_new(AST_STRING_LITERAL, NULL)` and `memcpy`s `token.length`
  bytes into `node->value`, then stamps `node->byte_length`. This
  replaces `ast_new(..., token.value)` whose `strncpy` would
  truncate at the first embedded NUL.

### Step 4: AST Pretty Printer

- `src/ast_pretty_printer.c`: `AST_STRING_LITERAL` now walks
  `byte_length` bytes and emits `\n`, `\t`, `\r`, `\\`, `\"`, `\0`
  as their source spellings. The `(length N)` suffix is preserved.

### Step 5: Token Printer

- `src/compiler.c`: `print_token_row` now special-cases
  `TOKEN_STRING_LITERAL`, building an escaped representation (using
  `Token.length`) before applying the existing 15/18-char width
  logic. Other token types still print via `strlen` exactly as
  before.

### Step 6: Code Generator

- `src/codegen.c`: `codegen_emit_string_pool` switched from
  `strlen(s->value)` to `s->byte_length`, so `"A\0B"` emits
  `db 65, 0, 66, 0` instead of being silently truncated at the
  embedded NUL.

### Step 7: Tests

Valid runtime tests under `test/valid/`, one per spec example:

- `test_string_literal_escape_n_newline__10.c`
- `test_string_literal_escape_r__13.c`
- `test_string_literal_escape_backslash__92.c`
- `test_string_literal_escape_quote__34.c`
- `test_string_literal_escape_null__0.c`
- `test_string_literal_escape_null_after__66.c`

Invalid test:

- `test/invalid/test_invalid_47__invalid_escape_sequence.c`
  for `"\x41"`.

Removed obsolete test:

- `test/invalid/test_invalid_45__escape_sequences_not_supported.c`
  was added in Stage 14-01 to enforce the
  "no escape sequences yet" rule. Stage 14-05 reverses that rule
  for `\n`, so the test no longer represents an error case and was
  removed.

Snapshot tests:

- `test/print_tokens/test_token_string_literal_escapes.c` + `.expected`
- `test/print_ast/test_stage_14_05_string_literal_escapes.c` + `.expected`
- `test/print_asm/test_stage_14_05_string_literal_escape_n.c` + `.expected`
- `test/print_asm/test_stage_14_05_string_literal_escape_null.c` + `.expected`

### Step 8: Documentation

- `docs/grammar.md`: extended the `<string_literal>` section with
  `<string_char>` and `<escape_sequence>` productions naming the
  six supported escapes.

## Final Results

Build clean. 389 / 389 tests pass (244 valid + 45 invalid + 22
print-AST + 73 print-tokens + 5 print-asm). No regressions.

## Session Flow

1. Read the Stage 14-05 spec and supporting templates / notes.
2. Reviewed the existing lexer, parser, AST, pretty-printer, token
   printer, and codegen string-pool paths.
3. Produced a kickoff with delta from Stage 14-04, ambiguities, and
   a step-by-step plan; saved under `docs/kickoffs/`.
4. Implemented changes in spec order: AST field → lexer → parser →
   pretty printer → token printer → codegen.
5. Built the compiler clean, then authored / captured runtime,
   token, AST, and ASM snapshot tests.
6. Identified an obsolete invalid test from Stage 14-01 and removed
   it because Stage 14-05 reverses its asserted behavior.
7. Updated `docs/grammar.md` to reflect the new escape grammar.
8. Ran the full suite to confirm 389/389 pass and recorded the
   milestone + transcript.

## Notes

- The spec's AST example (`StringLiteral: "A\n"`) omits the
  `(length N)` suffix added in Stage 14-02. The text "should *also*
  escape" was read as additive, so `(length N)` was preserved.
- "Escape printable characters" in the spec was read as a typo for
  "escape non-printable characters", consistent with the stated
  goal of keeping output line-oriented.
- `byte_length` was added to `ASTNode` rather than re-using
  `full_type->length` because Stage 14-04 rewrites `full_type` to
  `char *` during expression codegen, dropping the length.
