void set_value(int *p) {
    *p = 12;
    return;
}

int main() {
    int x;
    set_value(&x);
    return x;
}
