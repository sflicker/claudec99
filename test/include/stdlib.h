/* stdlib.h */
#ifndef CLAUDEC99_STDLIB_H
#define CLAUDEC99_STDLIB_H

#include <stddef.h>

void *malloc(size_t);
void *realloc(void *, size_t);
void *calloc(size_t nmemb, size_t size);
void free(void *);
void exit(int status);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
#endif
