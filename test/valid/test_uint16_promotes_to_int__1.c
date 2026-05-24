#include <stdint.h>

int main(void) {
    uint16_t s;
    int x;

    s = 65535U;
    x = s + 1;

    return x == 65536;
}
