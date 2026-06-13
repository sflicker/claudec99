#include <string.h>
int main(void) {
    char *s;
    s = "error: "
        "expected token";
    return strcmp(s, "error: expected token") == 0;
}
