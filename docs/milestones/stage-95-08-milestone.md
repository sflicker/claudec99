# Milestone Summary

## Stage 95-08: String Literal and Token Text Storage Refactor - Complete

stage-95-08 replaces the fixed-size `char value[MAX_NAME_LEN]` buffer in the `Token` struct with pointer-plus-length fields that reference lexer-owned string storage, eliminating the 255-byte string literal limit while keeping tokens fixed-size and passable by value.

- **Tokenizer**: `Token` struct redefined with `const char *lexeme`, `size_t lexeme_len`, `const char *value`, `size_t value_len` fields. Lexer adds `Vec str_pool` field to track heap-allocated string copies. New `lexer_store_bytes()` helper allocates individual copies and tracks them for cleanup via `lexer_free()`.
- **String handling**: String literals decoded through temporary `StrBuf`, enabling strings beyond 255 bytes and supporting embedded NUL bytes via `value_len`. Operators/keywords use static string literals; identifiers and numbers use `lexer_store_bytes()`.
- **Codegen**: Updated `tok->length` references to `(int)tok->value_len` where length information is needed.
- **Bootstrap defects fixed**: Two latent defects found during self-hosting:
  1. `*(char **)vec_get(...)` single-line cast-dereference not parseable by C0 parser → split into two statements.
  2. `strbuf_append_char` capacity overflow check had signed division bug (`(size_t)-1/2`) → fixed with local `size_t` variables.
- **Tests**: 4 new valid tests added (long strings, embedded NUL, escape sequences, adjacent literals). All 1478 tests pass at C0, C1, and C2 (up from 1474). Bootstrap cycle: C0→C1→C2 with versions `00.02.00950800.00731/00732/00733`.
- **Docs**: README updated through Stage 95-08 with 255-byte limit removed, aggregate test totals updated.
- **Notes**: `MAX_NAME_LEN` constant remains in `include/constants.h` (now applies to ASTNode only, not Token). Design trades per-string malloc vs arena for pointer stability across parser lookahead save/restore patterns.
