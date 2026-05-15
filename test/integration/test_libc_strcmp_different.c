int strcmp(const char *a, const char *b);
int puts(const char *);

int main() {
    char *a = "ALPHA";
    char *b = "BETA";

    int result = strcmp(a, b);

    if (result != 0) {
        puts("DIFFERENT");
    } else {
        puts("SAME");
    }
    return 0;
}
