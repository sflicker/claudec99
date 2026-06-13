static double scale(int n, double factor) { return n * factor; }
int main(void) {
    double r = scale(6, 7.0);
    return (int)r - 42;
}
