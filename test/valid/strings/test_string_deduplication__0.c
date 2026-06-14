int main() {
    char * a = "test";
    char * b = "test";
    return a == b;            // should return 1 if interned, 0 if not
}
