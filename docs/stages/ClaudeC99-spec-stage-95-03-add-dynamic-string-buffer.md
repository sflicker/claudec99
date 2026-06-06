# ClaudeC99 Stage 95-03 add dynamic byte/string buffer

## Task
Add a separate helper for character buffers
Example
```C
typedef struct StrBuf {
    char * data;
    size_t len;
    size_t cap;
} StrBuf;
```

Core operations
```C
void strbuf_init(StrBuf *b);
void strbuf_free(StrBuf *b);
int strbuf_reserve(StrBuf *b, size_t min_cap);
int strbuf_append_char(StrBuf *b, char c);
int strbuf_append_str(StrBuf *b, const char * s);
int strbuf_append_n(StrBuf *b, const char * s, size_t n);
int strbuf_null_terminate(StrBuf * b);
```

## Tests
Create similar style tests as used for the Vector module. Ensure adaqute test coverage.