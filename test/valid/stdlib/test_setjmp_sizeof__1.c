#include <setjmp.h>

int main(void) {
    jmp_buf env;
    return sizeof(env) > 0;
}
