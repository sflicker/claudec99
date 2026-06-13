int main(void) {
    float f = -3.0f;
    double d = -f;    /* double -(-3.0f) = 3.0 */
    return (int)d - 3;
}
