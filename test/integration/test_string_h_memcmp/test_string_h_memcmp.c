#include <string.h>

int main(void) {
    char a[] = "abc";
    char b[] = "abc";
    char c[] = "abd";
    if (memcmp(a, b, 3) != 0)
        return 1;
    if (memcmp(a, c, 3) >= 0)
        return 2;
    return 0;
}
