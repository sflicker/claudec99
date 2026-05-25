#include <stdio.h>
#include <string.h>

int main(void) {
    FILE *f;
    char buf[16];
    char *result;

    f = fopen("out.txt", "w");
    if (f == 0) {
        return 1;
    }

    fprintf(f, "%d\n", 42);
    fclose(f);

    f = fopen("out.txt", "r");
    if (f == 0) {
        return 2;
    }

    result = fgets(buf, sizeof(buf), f);
    fclose(f);

    if (result == 0) {
        return 3;
    }

    return strcmp(buf, "42\n") == 0;
}
