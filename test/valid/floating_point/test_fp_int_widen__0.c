int main(void) {
    int n = 7;
    double d = n + 0.5;   /* int 7 widened to double */
    return (int)(d * 2.0) - 15;
}
