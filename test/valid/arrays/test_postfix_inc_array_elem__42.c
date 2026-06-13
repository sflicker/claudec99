/* Stage 80: postfix increment on an array element lvalue.
 * xs[1] starts at 41, ++ makes it 42. */
int main(void) {
    int xs[2];

    xs[1] = 41;
    xs[1]++;

    return xs[1];
}
