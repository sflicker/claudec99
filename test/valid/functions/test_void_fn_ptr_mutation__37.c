void set_value(int *p) {
    *p = 37;
}

int main() {
    int x;
    set_value(&x);
    return x;
}
