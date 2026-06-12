/* stdio.h */
#ifndef CLAUDEC99_STDIO_H
#define CLAUDEC99_STDIO_H

#include <stddef.h>
#include <stdarg.h>

typedef long fpos_t;
typedef struct FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define BUFSIZ       8192
#define FOPEN_MAX    16
#define FILENAME_MAX 4096
#define L_tmpnam     20
#define TMP_MAX      238328

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

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

/* file operations */
int   remove(const char *filename);
int   rename(const char *old, const char *nw);
FILE *tmpfile(void);
char *tmpnam(char *s);

/* open / control */
FILE *freopen(const char * restrict filename,
              const char * restrict mode, FILE * restrict stream);
int   fflush(FILE *stream);
void  setbuf(FILE * restrict stream, char * restrict buf);
int   setvbuf(FILE * restrict stream, char * restrict buf,
              int mode, size_t size);

/* formatted output */
int sprintf(char * restrict s, const char * restrict format, ...);
int vsprintf(char * restrict s, const char * restrict format, va_list arg);

/* formatted input */
int scanf(const char * restrict format, ...);
int fscanf(FILE * restrict stream, const char * restrict format, ...);
int sscanf(const char * restrict s, const char * restrict format, ...);
int vscanf(const char * restrict format, va_list arg);
int vfscanf(FILE * restrict stream, const char * restrict format, va_list arg);
int vsscanf(const char * restrict s, const char * restrict format, va_list arg);

/* character output */
int fputc(int c, FILE *stream);
int fputs(const char * restrict s, FILE * restrict stream);
int putc(int c, FILE *stream);

/* character input */
int   getc(FILE *stream);
int   getchar(void);
char *gets(char *s);
int   ungetc(int c, FILE *stream);

/* binary I/O */
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb,
              FILE * restrict stream);

/* positioning */
int   fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int   fsetpos(FILE * restrict stream, const fpos_t *pos);
void  rewind(FILE *stream);

/* error */
void clearerr(FILE *stream);
int  feof(FILE *stream);
int  ferror(FILE *stream);
void perror(const char *s);

#endif
