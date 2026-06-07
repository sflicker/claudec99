# Stage 95-08 Kickoff: String Literal and Token Text Storage Refactor

## Spec Summary

Stage 95-08 eliminates the fixed-size `char value[MAX_NAME_LEN]` buffer from the `Token` struct by replacing it with pointer-plus-length fields that reference lexer-owned storage. This removes the arbitrary 127-byte limitation on token text (string literals, identifiers, etc.) while keeping tokens fixed-size and passable by value.

**Key design principles:**
- Tokens are fixed-size and passable by value
- Token text is owned by the lexer's arena (not by the token)
- Shallow token copies are safe
- Embedded NUL bytes in string literals are preserved via `value_len`

**The new Token fields:**
- `const char *lexeme` + `size_t lexeme_len` — raw source text spelling (e.g., `"abc\n"` with backslash-n)
- `const char *value` + `size_t value_len` — cooked/decoded value (e.g., bytes: a b c newline)

**Ownership model:**
- Lexer owns a `StrBuf arena` containing all token text
- Tokens reference this arena without owning it
- Text is freed only when the lexer is destroyed

## Tokenizer Changes

- `include/token.h`:
  - Remove `char value[MAX_NAME_LEN]` (currently 128 bytes)
  - Remove `int length` field
  - Add `const char *lexeme` field
  - Add `size_t lexeme_len` field
  - Add `const char *value` field
  - Add `size_t value_len` field

- `include/lexer.h`:
  - Add `StrBuf arena` field to hold all token text
  - Include `strbuf.h`

- `src/lexer.c`:
  - Add `lexer_store_bytes(Lexer *lex, const char *bytes, size_t len)` helper to copy bytes into the arena
  - Rewrite all token value assignments to use arena storage
  - **String literals**: Decode into temporary StrBuf, copy decoded bytes into arena, store lexeme separately
  - **Operators/punctuation**: Use static string literals (value may be left NULL; type determines operator text)
  - **Identifiers**: Store in arena
  - **Numbers**: Store in arena
  - String literal decoding flow:
    1. Create temporary `StrBuf decoded`
    2. Iterate through source, appending escaped chars to `decoded`
    3. Call `lexer_store_bytes` to copy into arena
    4. Free temporary buffer
  - Set both `value` and `value_len` for all tokens

## Parser Changes

- `src/parser.c`:
  - Replace `token.length` with `(int)token.value_len` where length is used

## AST Changes

None.

## Code Generation Changes

None.

## Test Plan

1. **Long string literal test** — Verify a string literal exceeding the old `MAX_NAME_LEN` (127 bytes) is handled correctly:
   ```c
   "very long string literal exceeding old MAX_NAME_LEN limit"
   ```

2. **Embedded NUL test** — Verify strings with embedded NUL bytes preserve their length:
   ```c
   "abc\0def"
   ```

3. **Escape sequence coverage** — Verify complex escape sequences decode correctly:
   ```c
   "line\nhex\x41octal\101"
   ```

4. **Adjacent string literals** — Verify string concatenation still works:
   ```c
   "adjacent" "strings"
   ```

5. **Existing token tests** — Verify all existing tokenizer and parser tests pass (backward compatibility)

6. **Self-host validation** — Verify bootstrap pipeline (C0 → C1 → C2) succeeds and produces identical binaries

## Implementation Order

1. Update `include/token.h` with new fields
2. Update `include/lexer.h` to add arena field
3. Add `lexer_store_bytes` helper in `src/lexer.c`
4. Rewrite string literal decoding in `src/lexer.c` to use temporary StrBuf + arena copy
5. Rewrite identifier/number/operator assignment in `src/lexer.c`
6. Update `src/parser.c` token.length usages
7. Update any other files referencing `token.length` (likely `src/compiler.c`)
8. Update `src/version.c` to `VERSION_STAGE = "00950800"`
9. Run full test suite after each file
10. Bootstrap validation (C0 → C1 → C2)

## Spec Issues Noted

1. Typo in struct definition: `size_t value;` should be `size_t value_len;`
2. Closing brace `} Token;` is missing from the struct block in the spec
3. Operator text storage is under-specified; design decision: use static string literals, leave `value` and `value_len` for non-operator tokens

## Ambiguities & Decisions

**Decision: Operator/punctuation handling**
- For operators like `==`, `!=`, etc., static string literals suffice
- Type field alone determines operator text
- The spec's example `get_token_text` helper is not required for this stage

**Decision: Lexeme field usage**
- Lexeme is stored for all tokens (raw source text with escapes visible)
- For identifiers and numbers, lexeme and value may point to identical arena storage
- Useful for future error reporting with original source text

**Decision: Arena lifecycle**
- Arena lives as long as the lexer
- Caller must not reference token text after lexer is destroyed
- This matches existing pointer semantics in the compiler
