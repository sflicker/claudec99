# ClaudeC99 stage 85 member-array to pointer decay

## Task
member array should decay to pointers when call a function accepting pointer argument

## Tests
```C
------- main test .c ------------
#include <stdio.h>
struct S {
    char name[32];
};

void f(char * str) {
    printf("%s\n", str);
}

int main(void) {
    struct S s = {"Hello"};
    f(s.name);  
    return 42;     // expect 42
}
----------------------------------

---------- .expected -------------
Hello
----------------------------------
```

```C
------- main test .c ------------
#include <stdio.h>
struct S {
    char name[32];
};

void f(char * str) {
    printf("%s\n", str);
}

int main(void) {
    struct S s = {"Hello"};
    struct S *p;
    p = &s;
    f(p->name);  
    return 42;     // expect 42
}
----------------------------------

---------- .expected -------------
Hello
----------------------------------
```
