# ClaudeC99 161 comparison with NULL through error when using --sysinclude flag

## Task
for the following program
```C
#include <stdio.h>

int main() {

    FILE * fp = NULL;
    if (fp == NULL) {
        printf("FP is NULL\n");
    }
    printf("Hello\n");
}
```

When compiled with the `--sysinclude` flag this throws an error
```
cc99 --sysinclude null_test.c
/home/scott/code/claude/claudec99/demos/null_test.c:6:30: error: incompatible pointer types in comparison
```

without the `--sysinclude` this works normally. Also casting NULL to (FILE*) also works. This also works when using `gcc`.

Investigate this issue and potentially fix it.
