typedef unsigned long size_t;
size_t strlen(const char *s);
int puts(const char *);

int main() {
    char *s = "HELLO";
    size_t len = strlen(s);
    if (len == 5) {
        puts("CORRECT");
    } else {
        puts("INCORRECT");
    }
    return 0;
}
