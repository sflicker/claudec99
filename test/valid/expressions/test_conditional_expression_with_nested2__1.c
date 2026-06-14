int main() {
    int a = 1 ? 10 : 20;
    int b = 0 ? 10 : 20;
    int c = (0 ? 1 : 2) ? 3 : 4;

    int g = (a == 10  && b == 20 && c == 3);

    return g;
}
