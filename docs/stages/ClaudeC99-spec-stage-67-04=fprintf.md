# ClaudeC99 stage 67-04 

## Task
  - add headers for file output and a test to verify.

## header file update. 
update the header file in <project-root>/test/include/stdio.h with this declaration
```C
int fprintf(FILE *, const char *, ...);
```

## Test
```C
----------- main test .c ----------
#include <stdio.h>
#include <string.h>

int main(void) {
    FILE *f;
    char buf[16];
    char *result;
    
    f = fopen("out.txt", "w");
    if (!f) {
        return 1;
    }
    
    fprintf(f, "%d\n", 42);
    fclose(f);
    
    f = fopen("out.txt", "r");
    if (!f) {
        return 2;
    }
    
    result = fgets(buf, sizeof(buf), f);
    fclose(f);
    
    if (!result) {
        return 3;
    }
    
    return strcmp("buf", "42\n") == 0;     // expect 1
}
```