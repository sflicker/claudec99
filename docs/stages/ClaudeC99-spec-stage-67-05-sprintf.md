# ClaudeC99 stage 67-05 add snprintf

## Task
add a declaration for snprintf to stdio.h

Add the following declaration to <project-root>/test/include/stdio.h
```C
int snprintf(char *, size_t, const char *, ...);
```

## Test
```C
#include <stdio.h>
#include <string.h>

int main(void) {
    char buf[16];
    
    snprintf(buf, sizeof(buf), "%d", 42);
    
    return strcmp(buf, "42") == 0;   // expect 1
}
```

