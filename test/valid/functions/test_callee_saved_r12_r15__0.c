static int double_it(int x) { return x * 2; }
static int triple_it(int x) { return x * 3; }
int main(void) {
    int a = double_it(5);
    int b = triple_it(7);
    return a + b - 31;
}
