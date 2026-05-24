# ClaudeC99 Stage 67-01 add headers for File I/O

## Task
Update the stdio.h in <project-root>/ust/include to the following
```C
typedef struct FILE FILE;
    
#define EOF (-1)
```

## Test
```C
#include <stdio.h>

int main(void) {
    FILE *f;
    f = 0;
    return f == 0 && EOF == -1;   // expect 1
}
```