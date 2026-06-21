/* Stage 159: function pointer typedef with array parameter (C99 §6.7.5.3p7).
 * The array decays to a pointer in the function signature. */
typedef int (*fn_t)(int argc, char *argv[]);

static int my_main(int argc, char *argv[]) {
    if (argc < 1) return -1;
    (void)argv;
    return argc;
}

int main(void) {
    fn_t f = my_main;
    char *args[] = {"prog", "arg1", "arg2"};
    int result = f(3, args);
    if (result != 3) return 1;
    return 42;
}
