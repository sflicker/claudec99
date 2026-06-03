#include <stdio.h>

int main() {
    FILE *f;
    long pos;

    f = fopen("input.txt", "r");
    if (!f) return 1;

    fseek(f, 0L, SEEK_END);
    pos = ftell(f);
    fclose(f);

    return pos > 0;      /* expect 1 */
}
