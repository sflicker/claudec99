int f(int x) {
    return x;
}

int main() {
    int a = 1;
    return f((a = 3, a + 4));
}
