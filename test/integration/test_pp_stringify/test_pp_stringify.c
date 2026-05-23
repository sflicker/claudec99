#include <stdio.h>

#define STR(x) #x

int main(void) {
    puts(STR(hello));
    return 0;
}
