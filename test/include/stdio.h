/* stdio.h */
#ifndef CLAUDEC99_STDIO_H
#define CLAUDEC99_STDIO_H

#include <stddef.h>
#include <stdarg.h>

typedef struct FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int fseek(FILE *, long, int);
long ftell(FILE *);
size_t fread(void *, size_t, size_t, FILE *);

int puts(const char *);
int printf(const char *, ...);

FILE *fopen(const char *, const char *);
int fclose(FILE *);
int fgetc(FILE *);
char *fgets(char *, int, FILE *);
int fprintf(FILE *, const char *, ...);
int snprintf(char *, size_t, const char *, ...);

int vfprintf(FILE *, const char *, va_list);
int vprintf(const char *, va_list);
int vsnprintf(char *, size_t, const char *, va_list);

int putchar(int);
#endif
