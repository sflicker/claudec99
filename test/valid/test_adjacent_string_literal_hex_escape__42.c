int main() {
    char *s;
    s = "\x41" "B";
    return s[0] + s[1] - 89;
}
