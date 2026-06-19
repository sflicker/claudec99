#define ATLEAST(maj, min) ((maj) * 100 + (min) >= 402)

int main(void) {
#if ATLEAST(4, 6)
    return 0;
#else
    return 1;
#endif
}
