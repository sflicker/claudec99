/* Stage 175: a "T *name[]" parameter (e.g. char *argv[]) decays to a
 * pointer-to-pointer per C99 6.7.5.3p7. Subscripting it must yield a
 * plain pointer (char *), not a non-pointer element, so the result can
 * be passed to a function whose parameter is declared as a plain
 * pointer (e.g. char *s). */
int str_len(char *s) {
    int n = 0;
    while (s[n] != 0) n = n + 1;
    return n;
}

int second_arg_len(int argc, char *argv[]) {
    if (argc < 2) return -1;
    return str_len(argv[1]);
}

int main(void) {
    char *args[] = {"prog", "hello"};
    return second_arg_len(2, args);
}
