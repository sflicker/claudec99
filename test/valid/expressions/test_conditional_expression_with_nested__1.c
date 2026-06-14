int main() {
    int a = 1 ? 10 : 20;
    int b = 0 ? 10 : 20;
    int c = (0 ? 1 : 2) ? 3 : 4;

    int d = a == 10;
    int e = b == 20;
    int f = c == 3;
    int g = d && e && f;

    return g;
}
