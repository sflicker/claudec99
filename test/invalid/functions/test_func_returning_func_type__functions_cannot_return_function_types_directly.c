/* int (f())(int) would declare f as returning a function type directly, which C99 forbids */
int (f())(int);

int main(void) {
    return 0;
}
