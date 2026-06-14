/* CC99-003: function-pointer calls with multiple double arguments */
typedef double (*Fn2)(double, double);
typedef double (*Fn3)(double, double, double);

double add2(double a, double b) { return a + b; }
double add3(double a, double b, double c) { return a + b + c; }

int main(void) {
    Fn2 p2 = add2;
    Fn3 p3 = add3;
    int r2 = (int)p2(1.0, 2.0);
    int r3 = (int)p3(1.0, 2.0, 3.0);
    return r2 + r3;
}
