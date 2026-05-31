/* stdlib.h */
#ifndef CLAUDEC99_STDLIB_H
#define CLAUDEC99_STDLIB_H

#include <stddef.h>

void *malloc(size_t);
void *realloc(void *, size_t);
void *calloc(size_t nmemb, size_t size);
void free(void *);
#endif
