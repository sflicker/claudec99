int is_positive(int x) {
    return x > 0;
}

int main() {
    int r = 0;
    if (is_positive(5)) {
        r = 42;
    }
    return r;
}
