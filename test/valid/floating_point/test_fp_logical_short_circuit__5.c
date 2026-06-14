/* CC99-005: logical && and || must short-circuit with FP operands */
int main(void) {
    double a = 0.0;
    double b = 2.0;
    int x = 0;

    if (a && (x = 1)) {
        x = 2;
    }

    if (b || (x = 3)) {
        x = x + 5;
    }

    return x;
}
