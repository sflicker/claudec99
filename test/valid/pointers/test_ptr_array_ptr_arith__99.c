int main() {
    char *names[1];
    char *p;

    names[0] = "abc";
    p = names[0] + 2;

    return *p; // expect 99: 'c'
}
