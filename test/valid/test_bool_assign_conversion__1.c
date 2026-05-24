#include <stdbool.h>

int main(void) {
    bool b;
    int x;
    int y;

    x = 3;
    y = -3;

    b = x + y;
    if (b != false) {
        return 0;
    }

    b = x + 1;
    return b != false;
}
