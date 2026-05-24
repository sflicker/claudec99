#include <stdio.h>

int main(void) {
    FILE *f;
    int c;

    f = fopen("input.txt", "r");
    if (f == 0) {
        return 1;
    }

    c = fgetc(f);
    fclose(f);

    return c;
}
