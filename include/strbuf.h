#ifndef CCOMPILER_STRBUF_H
#define CCOMPILER_STRBUF_H

#include <stddef.h>

/* Stage 95-03: dynamic character/string buffer.
 *
 * Stores raw bytes.  Growth policy: initial capacity 8, doubled on each
 * reallocation.  Allocation failure is a fatal internal error.
 * strbuf_null_terminate writes '\0' at data[len] without incrementing len,
 * making data a valid C string while keeping len the character count. */
typedef struct {
    char   *data;
    size_t  len;
    size_t  cap;
} StrBuf;

/* Initialize b to an empty buffer.  No memory is allocated until first use. */
void strbuf_init(StrBuf *b);

/* Release the backing store and zero b. */
void strbuf_free(StrBuf *b);

/* Ensure at least min_cap bytes can be stored without reallocation.
 * Returns 1 on success; exits on failure. */
int strbuf_reserve(StrBuf *b, size_t min_cap);

/* Append a single character.  Returns 1 on success; exits on failure. */
int strbuf_append_char(StrBuf *b, char c);

/* Append the null-terminated string s (not including the terminator).
 * Returns 1 on success; exits on failure. */
int strbuf_append_str(StrBuf *b, const char *s);

/* Append exactly n bytes from s.
 * Returns 1 on success; exits on failure. */
int strbuf_append_n(StrBuf *b, const char *s, size_t n);

/* Write '\0' at data[len] without incrementing len.
 * After this call, data is a valid null-terminated C string.
 * Returns 1 on success; exits on failure. */
int strbuf_null_terminate(StrBuf *b);

#endif /* CCOMPILER_STRBUF_H */
