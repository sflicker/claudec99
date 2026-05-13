/* Stage 40: unsigned > comparison. UINT_MAX > 1 is true. */
int main() {
    unsigned int big = 4294967295;   /* UINT_MAX */
    unsigned int small = 1;
    if (big > small)
        return 42;
    return 1;
}
