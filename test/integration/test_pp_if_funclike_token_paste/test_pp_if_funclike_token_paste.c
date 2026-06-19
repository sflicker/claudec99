#define CAT(a, b) a ## b
#define CAT_VAL1 1
#define CAT_VAL0 0

int main(void) {
    /* CAT(CAT_VAL, 1) expands to CAT_VAL1 which is 1 */
#if CAT(CAT_VAL, 1)
    return 0;
#else
    return 1;
#endif
}
