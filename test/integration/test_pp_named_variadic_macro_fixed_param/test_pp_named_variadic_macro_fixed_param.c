#include <stdio.h>

/* GNU named variadic macro parameter after a fixed parameter, the common
 * eprintf-style idiom: #define LOG(fmt, args...) printf(fmt, args) */
#define LOG(fmt, args...) printf(fmt, args)

int main(void) {
    LOG("%d-%d\n", 3, 4);
    return 0;
}
