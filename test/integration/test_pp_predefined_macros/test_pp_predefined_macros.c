#include "helper.h"
int main(void) {
    if (fileTest() != 0) return 1;
    if (lineTest() != 0) return 2;
    return 0;
}
