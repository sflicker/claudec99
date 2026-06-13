extern int (*fp)(int);
long (*fp)(int);        /* INVALID: return type mismatch */

int main() {
    return 0;
}
