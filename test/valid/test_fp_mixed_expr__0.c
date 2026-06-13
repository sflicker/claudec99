int main(void) {
    float  f = 2.0f;
    double d = 3.0;
    double r = f + d;   /* float widened to double */
    return (int)r - 5;
}
