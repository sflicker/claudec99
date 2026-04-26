int set(int *p) {
    *p = 9;
    return 0;
}

int main() {
    int x = 1;
    set(&x);
    return x;
}
