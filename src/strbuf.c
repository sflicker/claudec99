#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strbuf.h"

static void strbuf_fatal(const char *msg) {
    fprintf(stderr, "internal error: %s\n", msg);
    exit(1);
}

void strbuf_init(StrBuf *b) {
    b->data = NULL;
    b->len  = 0;
    b->cap  = 0;
}

void strbuf_free(StrBuf *b) {
    free(b->data);
    b->data = NULL;
    b->len  = 0;
    b->cap  = 0;
}

int strbuf_reserve(StrBuf *b, size_t min_cap) {
    if (b->cap >= min_cap)
        return 1;
    char *p = (char *)realloc(b->data, min_cap);
    if (!p)
        strbuf_fatal("strbuf_reserve: out of memory");
    b->data = p;
    b->cap  = min_cap;
    return 1;
}

int strbuf_append_char(StrBuf *b, char c) {
    if (b->len >= b->cap) {
        size_t new_cap;
        if (b->cap == 0) {
            new_cap = 8;
        } else {
            if (b->cap > (size_t)-1 / 2)
                strbuf_fatal("strbuf_append_char: capacity overflow");
            new_cap = b->cap * 2;
        }
        strbuf_reserve(b, new_cap);
    }
    b->data[b->len++] = c;
    return 1;
}

int strbuf_append_n(StrBuf *b, const char *s, size_t n) {
    if (n == 0)
        return 1;
    /* Ensure enough capacity for len + n bytes. */
    size_t needed = b->len + n;
    if (needed < b->len)
        strbuf_fatal("strbuf_append_n: size overflow");
    if (needed > b->cap) {
        size_t new_cap = b->cap == 0 ? 8 : b->cap;
        while (new_cap < needed) {
            if (new_cap > (size_t)-1 / 2)
                strbuf_fatal("strbuf_append_n: capacity overflow");
            new_cap *= 2;
        }
        strbuf_reserve(b, new_cap);
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
    return 1;
}

int strbuf_append_str(StrBuf *b, const char *s) {
    return strbuf_append_n(b, s, strlen(s));
}

int strbuf_null_terminate(StrBuf *b) {
    /* Need one extra byte beyond len for the '\0'. */
    if (b->len >= b->cap) {
        size_t new_cap;
        if (b->cap == 0) {
            new_cap = 8;
        } else {
            if (b->cap > (size_t)-1 / 2)
                strbuf_fatal("strbuf_null_terminate: capacity overflow");
            new_cap = b->cap * 2;
        }
        strbuf_reserve(b, new_cap);
    }
    b->data[b->len] = '\0';
    return 1;
}
