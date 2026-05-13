/* Stage 40: unsigned integer division.
 * 0xFFFFFFFF / 2 = 2147483647 (unsigned), not -1 (signed). */
int main() {
    unsigned int a = 4294967294;   /* 0xFFFFFFFE */
    unsigned int b = 2;
    unsigned int q = a / b;
    if (q == 2147483647)
        return 42;
    return 1;
}
