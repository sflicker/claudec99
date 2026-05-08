int main() {
    int (*fp)(int);
    return fp(3);   /* INVALID: call through function pointer not supported */
}
