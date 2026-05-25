/* string.h */
#ifndef CLAUDEC99_STRING_H
#define CLAUDEC99_STRING_H

#include <stddef.h>

int strcmp(const char *, const char *);
size_t strlen(const char *);
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
char *strchr(const char *, int);

#endif
