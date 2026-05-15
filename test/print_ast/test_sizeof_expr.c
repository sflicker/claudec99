int main() {
    int x = 0;
    long a = sizeof x;
    long b = sizeof(x + 1);
    return a + b;
}
