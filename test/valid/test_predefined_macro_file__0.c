int strlen(const char *s);
int main() {
    return strlen(__FILE__) > 0 ? 0 : 1;
}
