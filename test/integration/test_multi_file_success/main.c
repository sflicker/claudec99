/* main.c: links with file_a.o and file_b.o to verify multi-file output. */

#include <stdio.h>

int answer_a(void);
int answer_b(void);

int main(void) {
    printf("%d\n", answer_a());
    printf("%d\n", answer_b());
    return 0;
}
