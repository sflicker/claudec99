int f(int x) {
    return x;
}

int main() {
    int x = 7;
    return f(&x);
}
