#define ALWAYS_ZERO(x) 0

int main(void) {
#if ALWAYS_ZERO(999)
    return 1;
#else
    return 0;
#endif
}
