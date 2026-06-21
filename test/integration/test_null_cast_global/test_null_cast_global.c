/* Stage 163: global pointer initialized with (void *)0 null pointer constant.
 * Reproduces the bug where NULL defined as ((void *)0) triggers
 * "non-constant initializer for global" at file scope. */
#define NULL ((void *)0)
int printf(const char *, ...);

typedef struct { int x; int y; } Point;

Point *gp = NULL;
int *gi = NULL;

int main(void) {
    if (gp == NULL)
        printf("gp is null\n");
    if (gi == (void *)0)
        printf("gi is null\n");
    return 0;
}
