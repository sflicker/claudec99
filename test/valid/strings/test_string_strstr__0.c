#include <string.h>
int main(void) {
    const char *s = "hello world";
    return (strstr(s, "world") == s + 6) ? 0 : 1;
}
