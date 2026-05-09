typedef int (*Fn)(int);

int inc(int x) {
    return x + 1;
}

int main() {
    Fn a, b;
    a = inc;
    b = a;
    return b(6);     /* expect 7 */
}
