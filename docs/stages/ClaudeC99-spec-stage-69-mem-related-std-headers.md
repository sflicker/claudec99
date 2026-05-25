# ClaudeC99 stage 69 - add memory related standard functions to controlled headers

## Task
Add the following headers to the appropriate header in <project-root>/test/include
```text
void realloc(void *, size_t);                    // add to stdlib.h
void *memcpy(void *, const void *, size_t);      // add to string.h
void *memset(void *, int, size_t);               // add to string.h
int memcmp(const void *, const void *, size_t);  // add to string.h
char * strchr(const char *, int);                // add to string.h

```

## Tests
Add appropriate tests that exercise the added function declarations to validate usage
