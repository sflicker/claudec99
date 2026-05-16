int puts(const char *);
int main(int argc, char **argv) {
    if (argc > 1) {
        puts(argv[1]);
    }
    return argc;
}
