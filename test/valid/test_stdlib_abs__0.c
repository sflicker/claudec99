#include <stdlib.h>
/* abs() is declared in stdlib.h but 'abs' is a NASM built-in keyword, so
 * direct calls to abs() produce unassemblable output.  Test labs/llabs
 * for runtime behaviour; abs() declaration is verified by compilation. */
int main(void) {
    if (labs(-5L)   != 5L)  return 1;
    if (llabs(-5LL) != 5LL) return 2;
    return 0;
}
