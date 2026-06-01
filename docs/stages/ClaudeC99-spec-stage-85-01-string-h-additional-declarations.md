# ClaudeC99 Stage 85-01 string.h additional declarations

## Add additonal declarations to string.h
  - `strncpy`
  - `strncat`
  - `strncmp`
  - `strcpy`
  - `strrchr`

## Updates
```C
----- <project-root>/test/include/string.h ----
 char * strncpy(char *, const char *, size_t);
 char * strncat(char * dest, const char * src, size_t);
 int strncmp(const char *, const char *, size_t);
 char * strcpy(char *, const char *);
 char * strrchr(char *, int);

 ## Tests
 Create tests that demostrate the usage of each declaration along with validating their presence.
 