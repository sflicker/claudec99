#include <stdio.h>
#include <string.h>

int main(void) {
    FILE *f;
    char buf[16];
    char *result;

    f = fopen("input.txt", "r");
    if (f == 0) {
        return 1;
    }

    result = fgets(buf, 16, f);
    fclose(f);

    if (result == 0) {
        return 2;
    }

    return strcmp(buf, "hello\n") == 0;
}
