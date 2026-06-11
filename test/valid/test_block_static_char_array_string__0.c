char *greeting(void) {
    static char msg[] = "hi";
    return msg;
}

int main() {
    char *s = greeting();
    return (s[0] == 'h' && s[1] == 'i' && s[2] == '\0') ? 0 : 1;
}
