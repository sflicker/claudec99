int first(int *p) {
    return p[0];
}

int main() {
    int a[2];
    a[0] = 42;
    return first(a);
}
