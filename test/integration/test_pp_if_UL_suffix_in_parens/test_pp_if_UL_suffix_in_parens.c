int main(void) {
#if (1UL << 31) > 0
    return 0;
#else
    return 1;
#endif
}
