int inc(int x) {
    return x + 1;
}

int main() {
    int (*fp)(int);
    fp = inc;
    return 0;   /* expect 0 */
}
