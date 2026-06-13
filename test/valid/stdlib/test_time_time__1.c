#include <time.h>

int main(void) {
    time_t t;
    t = time(0);
    return t != 0;
}
