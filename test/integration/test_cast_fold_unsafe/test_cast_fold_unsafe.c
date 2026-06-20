#include <stdio.h>
int main(void) {
    /* (char)300: value 300 does not fit in signed char (-128..127);
       optimizer leaves the cast; codegen truncates to 44 */
    char a = (char)300;
    /* (unsigned char)(-1): value -1 does not fit in 0..255 unsigned byte;
       codegen produces 255 */
    unsigned char b = (unsigned char)(-1);
    printf("%d %d\n", (int)a, (int)b);
    return 0;
}
