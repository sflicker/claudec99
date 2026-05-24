# ClaudeC99 stage 67-03 line input with fgets

## Task
Add fgets to stdio.h header and validate

Add to <project-root>/test/include/stdio.h
```C
char *fgets(char *, int, FILE *);
```

## Test
```C
--------- input.txt ----------
hello

------------------------------

------ main test .c ----------
#include <stdio.h> 
#include <string.h>

int main(void) {
    FILE *f;
    char buf[16];
    char * result;
    
    f = fopen("input.txt", "r");
    if (!f) {
        return 1;
    }
    
    result = fget(buf, 16, f);
    fclose(f);
    
    if (!result) {
        return 2;
    }
    
    return strcmp(buf, "hello\n") == 0;
}
```
