/* Stage 80: prefix ++ yields the new value.
 * x = 41; y = ++x  -> y = 42. */
int main(void) {
    int x;
    int y;

    x = 41;
    y = ++x;

    return y;
}
