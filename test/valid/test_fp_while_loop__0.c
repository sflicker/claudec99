int main(void) {
    double sum = 0.0;
    double i   = 1.0;
    while (i <= 10.0) {
        sum = sum + i;
        i   = i + 1.0;
    }
    return (int)sum - 55;   /* 1+2+...+10 = 55 */
}
