# ClaudeC99  Stage 58 - controlled standard-style test headers

## Task
  - Add a controlled test include directory at `test/include` containing small standard-style headers for integration tests.
  - These headers are not the compiler's internal headers. They are test-facing headers used by programs compiled by ClaudeC99.
  - To access these files from tests a -I/test/include option will be placed in the tests .cflags file.
  - The tests will access the includes using an angled bracket include.

## Out of Scope
  - read host /usr/include
  - full libc headers
  - platform specific declarations
  - compiler attributes
  - builtin functions
  - errno
  - FILE
  - fopen / fclose /fread
  - full stdarg.h

Here are the controlled headers to be placed in the `test/include` directory

```C
-------- stddef.h --------
/* stddef.h */
#ifndef CLAUDEC99_STDDEF_H
#define CLAUDEC99_STDDEF_H

typedef unsigned long size_t;
#define NULL 0
#endif
--------------------------
```

```C
------- stdio.h -----
/* stdio.h */
#ifndef CLAUDEC99_STDIO_H
#define CLUADEC99_STDIO_H

int puts(const char *);
int printf(const char *, ...);
#endif
```

```C
--------- string.h ----
/* string.h */
#ifndef CLAUDEC99_STRING_H
#define CLAUDEC99_STRING_H

#include <stddef.h>

int strcmp(const char *, const char *);
size_t strlen(const char *);

#endif 
------------------------
```

```C
---------- stdlib.h ---------
#ifndef CLAUDEC99_STDLIB_H
#define CLAUDEC99_STDLIB_H

#include <stddef.h>

void *malloc(size_t);
void free(void *);
#endif
----------------------------
```

## Tests

test stdio.h / puts
```C
------- main .c file -------
#include <stdio.h>

int main(void) {
    puts("hello, world");
    return 0;  //expect 0
}
---------------------------

------ .cflags ------------
-I/test/include
---------------------------

----- .expected -----------
hello, world
---------------------------
```

test stdio.h / printf
```C
------- main .c file -------
#include <stdio.h>

int main(void) {
    printf("hello, world");
    return 0;  //expect 0
}
---------------------------

------ .cflags ------------
-I/test/include
---------------------------

----- .expected -----------
hello, world
---------------------------
```

test stddef.h size_t
```C
------- main.c file --------
#include <stddef.h>

int main(void) {
    size_t x;
    x = sizeof(int);
    return x == 4;       // expect 1
}
----------------------------

------- .cflags -----------
-I/test/include
---------------------------
```

test stddef.h NULL
```C
------- main .c file --------
#include <stddef.h>

int main(void) {
    int *p;
    p = NULL;
    return p == 0;       // expect 1
}
----------------------------

------- .cflags -----------
-I/test/include
---------------------------
```

test string.h strcmp
```C
------------- main .c file -------
#include <string.h>

int main(void) {
   return strcmp("abc", "abc");  // expect 0;
}
----------------------------------

------------- .cflags ------------
-I/test/include
----------------------------------
```

test string.h strlen
```C
-------------- main .c file----------
#include <string.h>

int main(void) {
    printf("%d", (int)strlen("hello"));
    return 0;    // expect 0
}
------------------------------------

------- .expected ------------------
5
------------------------------------

---------- .cflags -----------------
-I/usr/include
------------------------------------
```

test stdlib.h malloc free
```C
---------- main .c file ----------
#include <stdlib.h>

int main(void) {
    int *p;
    p = malloc(sizeof(int));
    *p = 42;
    int value = *p;
    free(p);
    return value;    // expect 42
---------------------------------

----------- .cflags -------------
-I/test/include
---------------------------------
```
