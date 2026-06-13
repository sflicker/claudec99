extern int (*fp)(int);
int (*fp)(long);        /* INVALID: parameter type mismatch */

int main() {
    return 0;
}
