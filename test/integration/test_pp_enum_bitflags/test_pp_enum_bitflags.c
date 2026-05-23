#include <stdio.h>

enum Perms {
    NONE = 0,
    READ = 1,
    WRITE = 2,
    EXEC = 4
};

int main(void) {
    int perms = READ | WRITE;
    printf("%d\n", perms);
    printf("%d\n", perms & READ);
    return 0;
}
