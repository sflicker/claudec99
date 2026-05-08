int inc(int x) {
    return x + 1;
}

int apply(int (*fp)(int), int x) {
    return fp(x);
}

int main() {
    return apply(inc, 6);   /* expect 7 */
}
