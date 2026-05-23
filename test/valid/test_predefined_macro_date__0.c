int strlen(const char *s);
int main() {
    return strlen(__DATE__) > 0 ? 0 : 1;
}
