int add(int a, int b) {
    return a + b;
}

int twice(int a) {
    return add(a, a);
}

int main() {
    return twice(21);
}
