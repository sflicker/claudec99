int strlen_cl(char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}
int main() {
    return strlen_cl((char[]){ 'h', 'i', '\0' });
}
