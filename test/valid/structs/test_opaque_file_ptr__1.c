#include <stdio.h>

int main(void) {
    FILE *f;
    f = 0;
    return f == 0 && EOF == -1;   // expect 1
}
