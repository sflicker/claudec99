# ClaudeC99 stage 81 header updates

## Task
  - update <project-root>/test/include/stdio.h to include putchar
  - update <project-root>/test/include/stdlib.h to include calloc
  - update <project-root>/include/constants.h to update several values

## Updates
```C
Add new declaration
--------- <project-root>/test/include/stdlib.h -----
void *calloc(size_t nmemb, size_t size);
----------------------------------------------------

Add new declaration
--------- <project-root>/test/include/stdio.h ------
int putchar(int);
----------------------------------------------------

Update existing constants
-------- <project-root>/include/constants.h --------
#define PARSER_MAX_FUNCTIONS 256
#define PARSER_MAX_GLOBALS 256
#define MAX_GLOBALS 256
#define MAX_LOCALS 256
```

## Tests

calloc test
```C
#include <stdlib.h>

int main(void) {
    int *p;
    
    p = calloc(4, sizeof(int));
    if (!p) {
        return 1;
    }
    
    if (p[0] != 0) {
        free(p);
        return 2;
    }
    
    if (p[1] != 0) {
        free(p);
        return 3;
    }
    
    p[2] = 42;
    
    if (p[2] != 42) {
        free(p);
        return 4;
    }
    
    free(p);
    return 0;     // expect 0
}

```

putchar test
```C
-------- main test .c ---------
#include <stdio.h> 

int main(void) {
    putchar('O');
    putchar('K');
    putchar('\n');
    return 0;  // expect status 0
}
--------- .expected ----------
OK
------------------------------
```

combined putchar/calloc test
```C
---------- main .c file ------------
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *p;
    
    p = calloc(3, sizeof(char));
    if (!p) {
        return 1;
    }
    
    p[0] = 'O';
    p[1] = 'K';
    
    putchar(p[0]);
    putchar(p[1]);
    putchar('\n');
    
    free(p);
    return 0;         // expect 0
}
----------------------------------

---------- .expected -------------
OK
----------------------------------
```