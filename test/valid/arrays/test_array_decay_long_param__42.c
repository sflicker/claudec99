long first(long *p) {
    return p[0];
}

int main() {
    long a[2];
    a[0] = 42;
    return first(a);
}
