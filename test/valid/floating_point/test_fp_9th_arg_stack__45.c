/* CC99-001: non-variadic double arguments beyond 8 (stack-passed) */
double sum9(
    double a1, double a2, double a3,
    double a4, double a5, double a6,
    double a7, double a8, double a9
) {
    return a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9;
}

int main(void) {
    return (int)sum9(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
}
