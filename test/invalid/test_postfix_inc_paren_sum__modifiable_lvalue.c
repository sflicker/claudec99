/* Stage 80: ++ requires a modifiable lvalue; a parenthesized arithmetic
 * expression is an rvalue and must be rejected. */
int main(void) {
    int a;
    int b;

    a = 1;
    b = 2;

    (a + b)++;

    return 0;
}
