# ClaudeC99 - 75-05 additional va_list tests

## Task
   - add additional additional va_list tests

## Tests
No variable args
```C
------------ main test .c --------------
#include <stdarg.h>
#include <stdio.h>

void printv(const char *fmt, va_list ap) {
    vprintf(fmt, ap);
}

void print(const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printv(fmt, ap);
    va_end(ap);
}

int main() {
    print("Hello\n");
    return 0;
}
---------------------------------------
-------------- .expected --------------
Hello\n
---------------------------------------
```

6 variable args
```C
------------ main test .c --------------
#include <stdarg.h>
#include <stdio.h>

void printv(const char *fmt, va_list ap) {
    vprintf(fmt, ap);
}

void print(const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printv(fmt, ap);
    va_end(ap);
}

int main() {
    print("%d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6);
    return 0;
}
---------------------------------------
-------------- .expected --------------
1 2 3 4 5 6\n
---------------------------------------
```

10 variable args
```C
------------ main test .c --------------
#include <stdarg.h>
#include <stdio.h>

void printv(const char *fmt, va_list ap) {
    vprintf(fmt, ap);
}

void print(const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printv(fmt, ap);
    va_end(ap);
}

int main() {
    print("%d %d %d %d %d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    return 0;
}
---------------------------------------
-------------- .expected --------------
1 2 3 4 5 6 7 8 9 10\n
---------------------------------------
```
