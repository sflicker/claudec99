int main(void) {
    double zero    = 0.0;
    double nonzero = 1.5;
    int a = !zero;      /* 1 */
    int b = !nonzero;   /* 0 */
    return (a == 1 && b == 0) ? 0 : 1;
}
