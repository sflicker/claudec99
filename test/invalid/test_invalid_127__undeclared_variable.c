int f(int x) {
    return x;
}

int main() {
    int (*fp)(int);
    fp = f;     /* INVALID: assigning function to function pointer not supported */
    return 0;
}
