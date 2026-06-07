# stage-95-08 Transcript: String Literal and Token Text Storage Refactor

## Summary

Stage 95-08 eliminates the fixed-size `char value[MAX_NAME_LEN]` buffer from the `Token` struct, replacing it with pointer-plus-length fields (`const char *value`, `size_t value_len`, `const char *lexeme`, `size_t lexeme_len`) that reference lexer-owned storage. This change removes the arbitrary 255-byte limitation on token text—particularly string literals—while keeping tokens fixed-size and safely passable by value.

The lexer implements a string pool via a `Vec str_pool` that tracks heap-allocated strings. A `lexer_store_bytes()` helper allocates individual copies (per-string malloc) and appends them to the pool; cleanup occurs via `lexer_free()` which iterates the pool to free all allocated memory. This design provides pointer stability across parser lookahead save/restore patterns, which would be problematic with a traditional arena allocator.

String literals are decoded through a temporary `StrBuf`, enabling strings beyond the old 255-byte limit and supporting embedded NUL bytes via `value_len`. Operators and keywords use static string literals. Identifiers and integer literals use `lexer_store_bytes()`.

## Changes Made

### Step 1: Token Structure

- Removed `char value[MAX_NAME_LEN]` and `int length` fields from `Token` struct.
- Added `const char *lexeme` and `size_t lexeme_len` fields for raw source spelling.
- Added `const char *value` and `size_t value_len` fields for decoded/cooked token text.
- Added `#include <stddef.h>` to `include/token.h` for `size_t` type.

### Step 2: Lexer Arena Infrastructure

- Added `Vec str_pool` field to `Lexer` struct in `include/lexer.h` to track heap-allocated string copies.
- Added `#include "vec.h"` to `include/lexer.h`.
- Implemented `lexer_store_bytes(Lexer *lex, const char *bytes, size_t len)` helper function that:
  - Allocates memory for `len + 1` bytes (adding space for null terminator).
  - Copies the bytes into the allocated buffer.
  - Appends the pointer to `lex->str_pool` via `vec_push`.
  - Returns the stable pointer to the caller.
- Updated `lexer_free()` to iterate through `str_pool` and free all allocated strings.

### Step 3: String Literal Decoding

- Rewrote string literal decoding flow in `src/lexer.c`:
  - Create temporary `StrBuf decoded` buffer.
  - Iterate through source characters, appending decoded/unescaped characters to `decoded`.
  - Call `lexer_store_bytes(lexer, decoded.data, decoded.len)` to copy the decoded bytes into lexer-owned storage.
  - Free the temporary buffer via `strbuf_free(&decoded)`.
  - Store the arena pointer in `token->value` and `decoded.len` in `token->value_len`.
  - Store raw source spelling in `token->lexeme` and its length in `token->lexeme_len`.

### Step 4: Operator and Keyword Handling

- Operators, punctuation, and keywords now reference static string literals or store NULL in `value` (type determines text).
- `value_len` remains set to the appropriate length for consistency.

### Step 5: Identifier and Number Handling

- Identifiers and integer literals allocate storage via `lexer_store_bytes()`.
- Both `lexeme` and `value` reference the same arena storage (no raw vs. decoded distinction).

### Step 6: Removed finalize() Function

- Deleted the `finalize()` helper that was used to null-terminate token values.
- String null-termination now occurs at allocation time in `lexer_store_bytes()`.

### Step 7: Parser Updates

- Replaced `token.length` with `(int)token.value_len` where token length is needed.
- Updated string literal handling in `str_init()` to use `value_len` instead of length.

### Step 8: Compiler Updates

- Updated `src/compiler.c` to use `(int)tok->value_len` instead of `tok->length`.

### Step 9: StrBuf Overflow Fix

- Fixed capacity overflow checks in `src/strbuf.c` (three functions: `strbuf_reserve`, `strbuf_append_char`, `strbuf_append_str`).
- Root cause: signed division on `(size_t)-1/2` emitted by C0 parser during bootstrap.
- Fix: Introduce local `size_t` variables to ensure unsigned division.

### Step 10: Version Update

- Updated `src/version.c`:
  - `VERSION_STAGE = "00950800"`
  - `VERSION_BUILTBY = "gcc_Ubuntu_13_3_0"`

### Step 11: Bootstrap Defect 1: Cast-Dereference

- **Issue**: C0 parser could not parse `*(char **)vec_get(...)` single-line expression (double-cast-dereference in one statement).
- **Location**: `src/lexer.c` in `lexer_store_bytes()` helper.
- **Fix**: Split into two statements:
  ```c
  char **ptr = (char **)vec_get(&lex->str_pool, lex->str_pool.len - 1);
  return *ptr;
  ```

### Step 12: Bootstrap Defect 2: Signed Division in StrBuf

- **Issue**: `strbuf_append_char` and related functions in `src/strbuf.c` emitted signed division (`idiv`) for the overflow check `(size_t)-1/2 >= buf->len + 1`, causing incorrect behavior.
- **Root cause**: Same as stage 95-05's `vec_push` defect.
- **Fix**: Rewrite overflow checks using local `size_t` variables to guarantee unsigned `div`:
  ```c
  size_t max_size = (size_t)-1 / 2;
  if (max_size >= buf->len + 1) { /* ... */ }
  ```
- **Functions affected**: `strbuf_reserve`, `strbuf_append_char`, `strbuf_append_str`.

### Step 13: Constants and Documentation

- Updated `include/constants.h` comment for `MAX_NAME_LEN` to reflect that it now applies only to `ASTNode` fields, not `Token`.

### Step 14: Tests

- Added 4 new valid tests:
  1. `test_string_very_long__118.c` — String literal exceeding old 255-byte limit.
  2. `test_string_embedded_nul_abc0def__100.c` — Embedded NUL byte preservation.
  3. `test_string_newline_hex_octal_escapes__10.c` — Complex escape sequence coverage.
  4. `test_string_adjacent_spec__115.c` — Adjacent string literal concatenation.

## Final Results

All 1478 tests pass (up from 1474 in stage 95-07):
- 165 unit tests
- 827 valid tests
- 237 invalid tests
- 82 integration tests
- 43 print_ast tests
- 100 print_tokens tests
- 21 print_asm tests

Bootstrap cycle: C0→C1→C2 successful with versions:
- C0: `00.02.00950800.00731` (GCC-built)
- C1: `00.02.00950800.00732` (C0-built)
- C2: `00.02.00950800.00733` (C1-built)

All tests pass at each stage. C1 and C2 are behavior-identical.

## Session Flow

1. Read stage spec and design documentation.
2. Reviewed current `Token` and `Lexer` struct definitions.
3. Reviewed `StrBuf` module from stage 95-03 and `Vec` module from stage 95-02.
4. Planned pointer-ownership model and string pool design (per-string malloc vs. arena).
5. Implemented Token struct changes in `include/token.h`.
6. Implemented Lexer arena infrastructure in `include/lexer.h` and `src/lexer.c`.
7. Rewrote string literal decoding in `src/lexer.c` to use temporary StrBuf + arena copy.
8. Updated identifier, number, and operator token assignments.
9. Updated parser and compiler references to `token.length`.
10. Fixed `strbuf.c` overflow checks (capacity calculation).
11. Updated version strings in `src/version.c`.
12. Built with GCC and ran test suite (1478 tests pass).
13. Ran bootstrap cycle (C0→C1→C2) and discovered two defects.
14. Fixed bootstrap defect 1 (cast-dereference split) in `lexer_store_bytes()`.
15. Fixed bootstrap defect 2 (signed division) in `strbuf.c` overflow checks.
16. Re-ran bootstrap cycle to confirm fixes.
17. Verified all tests pass at C0, C1, and C2.
18. Generated milestone summary and transcript.

## Notes

**Design Decision: Per-String Malloc vs. Arena**

The implementation uses a `Vec str_pool` tracking individual `malloc`-allocated strings rather than a traditional arena allocator. This choice prioritizes pointer stability across parser lookahead save/restore patterns. If the parser saves a token and later restores it after tentative parsing (e.g., during type specifier lookahead), the token's pointers must remain valid. A traditional arena allocator would invalidate pointers on realloc; per-string malloc preserves them.

**Constants Retained**

`MAX_NAME_LEN` remains in `include/constants.h` but now applies only to fixed-size `char` arrays in `ASTNode` and other non-token structures (e.g., assembly labels). Token text is no longer constrained by this constant.

**String Ownership Model**

Tokens do not own text; the lexer's `str_pool` does. Token copies are shallow and safe. Text is freed only when the lexer is destroyed. This model is safe because the parser typically processes one token at a time and does not retain references beyond the lexer's lifetime.
