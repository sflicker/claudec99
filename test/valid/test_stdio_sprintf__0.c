#include <stdio.h>
#include <string.h>
int main(void) {
    char buf[32];
    sprintf(buf, "%d-%s", 42, "ok");
    return strcmp(buf, "42-ok");
}
