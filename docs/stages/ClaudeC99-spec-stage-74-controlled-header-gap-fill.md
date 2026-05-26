# ClaudeC99 stage 74 - controlled header gap fill

## Task
  - add missing standard header files to the controlled headers subset in <project-root>/test/include

Add the headers with the following declarations/macros to be placed in <project-root>/test/include

```C
------------ ctype.h ----------
#ifndef CLAUDEC99_CTYPE_H
#define CLAUDEC99_CTYPE_H

int isalnum(int);
int isalpha(int);
int isdigit(int);
int isspace(int);
int isxdigit(int);

int islower(int);
int isupper(int);

int tolower(int);
int toupper(int);
#endif
-------------------------------------
```

```C
--------- errno.h -------------------
#ifndef CLAUDEC99_ERRNO_H
#define CLAUDEC99_ERRNO_H

int *__errno_location(void);

#define errno(*__errno_location())

#define EPERM  1
#define ENOENT 2
#define ESRCH  3
#define EINTR  4
#define EIO    5
#define ENOMEM 12
#define EACCES 13
#define EINIVAL 22
#define ERANGE  34
#endif
-----------------------------------
```

```C
--------------- time.h -----------------
#ifndef CLAUDEC99_TIME_H
#define CLAUDEC99_TIME_H

typedef log time_t;
typedef long clock_t;

#define CLOCKS_PER_SEC 1000000L

time_t time(time_t *);
clock_t clock(void);
#endif
```

```C 
------------- setjmp.h ----------------
#ifndef CLAUDEC99_SETJMP_H
#define CLAUDEC99_SETJMP_H

typedef long jmp_buf[32];

int setjmp(jmp_buf);
void longjmp(jmp_buf, int);
#endif
```

## out of scope
  - full host /usr/include compatibility
  - locale-sensitive ctype behavior
  - full errno portability
  - struct tm
  - strftime
  - localtime
  - mktime
  - full setjmp ABI modeling
  - signal masks
  - thread-local errno portability outside Linux/glibc

## Tests
```C
#include <ctype.h>

int main(void) {
    return isdigit('7') != 0;   // expect: 1
}
```

```C
#include <ctype.h>

int main(void) {
    return isalpha('A'); != 0;  // expect 1
}

```

```C
#include <ctype.h>

int main(void) {
    return toupper('a') == 'A';    // expect 1
}
```

```C
#include <ctype.h>

int main(void) {
    return isspace('\n') != 0;   // expect 1
}
```

```C
#include <errno.h>

int main(void) {
    errno = 0;
    errno = EINVAL;
    return errno == EINVAL;     // expect 1
}
```

```C
#include <errno.h>

int main(void) {
    return ENOENT == 2;  // expect 1
}
```

```C
#include <time.h>

int main(void) {
    time_t t;
    t = time(0);
    return t != 0;    // expect 1
}
```

```C
#include <setjmp.h>

int main(void) {
    jmp_buf env;
    return sizeof(env) >0;   // expect 1
}
```