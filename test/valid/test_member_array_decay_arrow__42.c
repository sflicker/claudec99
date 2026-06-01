#include <stdio.h>

/* Stage 85: an array-typed struct member decays to a pointer to its
 * first element when passed to a function taking a pointer argument
 * (arrow access through a pointer-to-struct). */
struct S {
    char name[32];
};

void f(char *str) {
    printf("%s\n", str);
}

int main(void) {
    struct S s = {"Hello"};
    struct S *p;
    p = &s;
    f(p->name);
    return 42;
}
