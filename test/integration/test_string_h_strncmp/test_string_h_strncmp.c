#include <string.h>

int main(void) {
    if (strncmp("abc", "abx", 2) != 0)
        return 1;
    if (strncmp("abc", "abd", 3) == 0)
        return 2;
    return 0;
}
