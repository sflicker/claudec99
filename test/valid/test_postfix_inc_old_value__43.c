/* Stage 80: postfix ++ yields the old value.
 * x = 41; y = x++  -> x = 42, y = 41; 42 + 41 - 40 = 43. */
int main(void) {
    int x;
    int y;

    x = 41;
    y = x++;

    return x + y - 40;
}
