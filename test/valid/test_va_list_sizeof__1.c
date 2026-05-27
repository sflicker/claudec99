#include <stdarg.h>

int main(void) {
    va_list ap;
    return sizeof(ap) > 0;     /* expect 1 */
}
