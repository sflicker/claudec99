#include <string.h>
int main() {
    char str[6];
    str[0] = 'H';
    str[1] = 'e';
    str[2] = '\x6c';
    str[3] = '\154';
    str[4] = 'o';
    str[5] = 0;

    return strcmp("Hello", str) == 0;
}
