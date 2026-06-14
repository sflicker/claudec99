// test Newton-Raphson to approximate sqrt(input)
// pass if |x^2 - 2| < 1e-10

double input = 2.0;
double TOL = 1e-6;

double double_abs(double x) { 
    if (x < 0) {
        return -x;
    }
    return x; 
}

int main() {
    double x = 1.0;        // initial guess

    for (int i = 0;i < 50;i++) {
//        _print(i);
        double fx = x*x - input;
        if (double_abs(x*x - input) < TOL) break;
        double dfx = input * x;
        x = x - fx /dfx;
//        _print(x);
    }

    // test if result is within tol
    if (double_abs(x*x - input) < TOL) {
        return 0;
    }
    else {  
        return 1;
    }
}
