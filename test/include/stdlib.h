/* stdlib.h */
#ifndef CLAUDEC99_STDLIB_H
#define CLAUDEC99_STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX     32767
#define MB_CUR_MAX   1

typedef struct { int       quot; int       rem; } div_t;
typedef struct { long      quot; long      rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;

void *malloc(size_t);
void *realloc(void *, size_t);
void *calloc(size_t nmemb, size_t size);
void free(void *);
void exit(int status);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);

/* process control */
void  abort(void);
int   atexit(void (*func)(void));
void  _Exit(int status);
int   system(const char *string);
char *getenv(const char *name);

/* random numbers */
int  rand(void);
void srand(unsigned int seed);

/* integer arithmetic */
int       abs(int j);
long      labs(long j);
long long llabs(long long j);
div_t     div(int numer, int denom);
ldiv_t    ldiv(long numer, long denom);
lldiv_t   lldiv(long long numer, long long denom);

/* string conversion */
int       atoi(const char *nptr);
long      atol(const char *nptr);
long long atoll(const char *nptr);
long long          strtoll(const char * restrict nptr,
                            char ** restrict endptr, int base);
unsigned long long strtoull(const char * restrict nptr,
                             char ** restrict endptr, int base);

/* searching and sorting */
void *bsearch(const void *key, const void *base,
              size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
void  qsort(void *base, size_t nmemb, size_t size,
            int (*compar)(const void *, const void *));

#endif
