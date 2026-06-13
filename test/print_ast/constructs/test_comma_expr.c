int main() {
    int a = 0;
    int b = 0;
    a = (a = 1, b = 2, a + b);
    return a;
}
