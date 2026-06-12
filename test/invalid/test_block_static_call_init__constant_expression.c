int g(void) { return 7; }
int main(void) {
    static int x = g();   /* INVALID: function call in static initializer */
    return x;
}
