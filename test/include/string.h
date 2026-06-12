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
char *strncpy(char *, const char *, size_t);
char *strncat(char *, const char *, size_t);
int strncmp(const char *, const char *, size_t);
char *strcpy(char *, const char *);
char *strrchr(char *, int);

void *memmove(void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);
char *strcat(char * restrict s1, const char * restrict s2);
size_t strcspn(const char *s1, const char *s2);
size_t strspn(const char *s1, const char *s2);
char *strpbrk(const char *s1, const char *s2);
char *strstr(const char *s1, const char *s2);
char *strtok(char * restrict s1, const char * restrict s2);
int   strcoll(const char *s1, const char *s2);
size_t strxfrm(char * restrict s1, const char * restrict s2, size_t n);
char *strerror(int errnum);

#endif
