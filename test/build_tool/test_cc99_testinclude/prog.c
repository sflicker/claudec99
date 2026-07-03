#include <stdio.h>

int main(void)
{
#ifdef CLAUDEC99_STDIO_H
    printf("stub\n");
#else
    printf("system\n");
#endif
    return 0;
}
