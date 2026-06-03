#include <stdio.h>
#include <string.h>

int main(void) {
    FILE *f;
    char buf[6];
    size_t n;

    f = fopen("input.txt", "r");
    if (!f) return 2;

    n = fread(buf, 1, 5, f);
    fclose(f);

    buf[5] = '\0';

    return n == 5 && strcmp(buf, "hello") == 0;    /* expect 1 */
}
