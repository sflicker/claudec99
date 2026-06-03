#include <string.h>
int main() {
    char *str = "He\x6c\154o";
    return strcmp("Hello", str) == 0;
}
