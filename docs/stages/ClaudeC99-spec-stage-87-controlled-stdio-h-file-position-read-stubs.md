# ClaudeC99 stage 87 Controlled stdio.h file position/read stubs

## Task 
add the following updates to <project-root>/test/include/stdio.h
```C
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int fseek(FILE *, long, int);
long ftell(FILE *);
size_t fread(void *, size_t, size_t, FILE *);
```

## Tests
fseek test
```C
------- test main .c file -----------
#include <stdio.h>

int main() {
    FILE *f;
    long pos;

    f = fopen("input.txt", "r");
    if (!f) return 1;

    fseek(f, 0L, SEEK_END);
    pos = ftell(f);
    fclose(f);

    return pos > 0;      // expect 1
}
--------------------------------------
----------- input.txt ----------------
This is a Test File
This is the second line.
--------------------------------------
```

fread test
```C
---------- test main .c --------------
#include <stdio.h>
#include <string.h>

int main(void) {
    FILE *f;
    char buf[6];
    size_t n;

    f = fopen("input.txt", "r");
    if (!f) return 2;

    n = fread(buf, 1, 5, f);
    fclose(f);

    buf[5] = '\0';

    return n == 5 && strcmp(buf, "hello") == 0;    // expect 1
}
----------------------------------------
-------- input.txt ---------------------
hello
----------------------------------------

