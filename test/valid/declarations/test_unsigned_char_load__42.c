/* Stage 40: unsigned char uses zero-extension on load.
 * 0xFF as unsigned char is 255, not -1. */
int main() {
    unsigned char c = 255;
    int x = c;      /* zero-extend: x should be 255, not -1 */
    if (x == 255)
        return 42;
    return 1;
}
