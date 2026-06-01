# ClaudeC99 stage 84-02 add exit() to stdlib.h

## Task
Update stdlib.h to include exit()

## Update
------- <project-root>/test/include/stdlib.h
void exit(int status)
---------------------------------------------

## Test
```C
#include <stdlib.h>

int main() {
    exit(42);         // expect 42
}
```
