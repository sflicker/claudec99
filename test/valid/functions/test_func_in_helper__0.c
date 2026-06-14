#include <stdio.h>
static void helper(void) { printf("%s\n", __func__); }
int main(void) { helper(); return 0; }
