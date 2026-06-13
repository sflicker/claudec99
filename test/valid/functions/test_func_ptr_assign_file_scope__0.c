int inc(int x) { return x + 1; }

int (*fp)(int) = inc;

int main() {
    return 0;   /* expect 0 */
}
