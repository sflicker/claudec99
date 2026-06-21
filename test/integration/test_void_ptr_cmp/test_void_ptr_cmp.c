/* Stage 161: void * is comparable with any object pointer type (C99 §6.5.9). */
int printf(const char *, ...);

int main(void) {
    int x = 42;
    int *ip = &x;
    void *vp = 0;

    if (ip != vp)
        printf("not null\n");

    vp = ip;

    if (ip == vp)
        printf("same\n");

    if (vp != 0)
        printf("non-null\n");

    return 0;
}
