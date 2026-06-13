int read_val(int *p) {
    return *p;
}

int main() {
    int (*fp)(int *) = read_val;
    return fp(3);   /* INVALID: expected pointer, got integer */
}
