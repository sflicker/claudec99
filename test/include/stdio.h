/* stdio.h */
#ifndef CLAUDEC99_STDIO_H
#define CLAUDEC99_STDIO_H

typedef struct FILE FILE;

#define EOF (-1)

int puts(const char *);
int printf(const char *, ...);

FILE *fopen(const char *, const char *);
int fclose(FILE *);
int fgetc(FILE *);
char *fgets(char *, int, FILE *);
int fprintf(FILE *, const char *, ...);
#endif
