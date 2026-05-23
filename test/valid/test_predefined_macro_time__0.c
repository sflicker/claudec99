int strlen(const char *s);
int main() {
    return strlen(__TIME__) > 0 ? 0 : 1;
}
