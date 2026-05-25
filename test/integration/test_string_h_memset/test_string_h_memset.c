#include <string.h>

int main(void) {
    char buf[4];
    memset(buf, 7, 4);
    if (buf[0] == 7 && buf[1] == 7 && buf[2] == 7 && buf[3] == 7)
        return 0;
    return 1;
}
