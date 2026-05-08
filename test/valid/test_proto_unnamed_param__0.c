/* Function prototype with unnamed parameters. */
int add(int, int);

int add(int a, int b) {
    return a + b;
}

int main() {
    return add(2, 0) - 2;   /* expect 0 */
}
