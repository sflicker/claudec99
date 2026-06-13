#include <stdint.h>

int main(void) {
    uint8_t c;
    int x;

    c = 255U;
    x = c + 1;

    return x == 256;
}
