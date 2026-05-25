#include <stdio.h>
#include <string.h>

int main(void) {
    char buf[16];

    snprintf(buf, sizeof(buf), "%d", 42);

    return strcmp(buf, "42") == 0;   /* expect 1 */
}
