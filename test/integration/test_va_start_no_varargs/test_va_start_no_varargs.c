#include <stdarg.h>
#include <stdio.h>

void printv(const char *fmt, va_list ap) {
    vprintf(fmt, ap);
}

void print(const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printv(fmt, ap);
    va_end(ap);
}

int main() {
    print("Hello\n");
    return 0;
}
