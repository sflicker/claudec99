/* No __cplusplus defined: ternary picks else-branch (defined __USE_ISOC11) */
#define __USE_ISOC11 1

int main(void) {
#if defined __cplusplus ? __cplusplus >= 201402L : defined __USE_ISOC11
    return 0;
#else
    return 1;
#endif
}
