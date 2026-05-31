# ClaudeC99 Stage 84 standard streams

## Task
add streams to controlled stdio.h

## Update
```C
-------- <project-root>/test/include/stdio.h -----
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
--------------------------------------------------

## Test

```C
#include <stdio.h>

int main(void) {
    fprintf(stdout, "out\n");
    fprintf(stderr, "err\n");
    return stdin != 0 && stdout != 0 && stderr != 0;  // expect 1
}