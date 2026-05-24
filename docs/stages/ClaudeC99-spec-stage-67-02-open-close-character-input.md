# ClaudeC99 stage 67-02 add file open and close and character input

## Task
  - add standard file io declarations to <project-root>/test/include stdio.h

Add these declarations to <project-root>/test/include/stdio.h
```C
FILE *fopen(const char *, const char *);
int fclose(FILE *);
int fgetc(FILE *);
```

## Test
this should be an integeration test
```C
----------- input.txt -------------
A
--------------------------------

---------- main .f file --------
#include <stdio.h>

int main(void) {
    FILE * f;
    int c;
    
    f = fopen("input.txt", "r");
    if (!f) {
       return 1;
    }
    
    c = fgetd(f);
    fclose(f);
    
    return c;   // expect 65
}
```