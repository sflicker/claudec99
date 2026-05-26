#include <errno.h>

int main(void) {
    errno = 0;
    errno = EINVAL;
    return errno == EINVAL;
}
