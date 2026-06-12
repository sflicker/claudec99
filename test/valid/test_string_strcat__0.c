#include <string.h>
int main(void) {
    char buf[16] = "hello";
    strcat(buf, " world");
    return strcmp(buf, "hello world");
}
