/* Stage 40: unsigned short uses zero-extension on load. */
int main() {
    unsigned short s = 65535;
    int x = s;      /* zero-extend: x should be 65535, not -1 */
    if (x == 65535)
        return 42;
    return 1;
}
