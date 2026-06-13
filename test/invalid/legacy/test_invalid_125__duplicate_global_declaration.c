static int (*fp)(int);
static int (*fp)(int);  /* INVALID: duplicate static definition */

int main() {
    return 0;
}
