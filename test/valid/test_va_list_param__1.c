#include <stdarg.h>

int helper(va_list ap) {
    return ap != 0;
}

int main(void) {
    va_list ap;
    return helper(ap);     /* expect 1 */
}
