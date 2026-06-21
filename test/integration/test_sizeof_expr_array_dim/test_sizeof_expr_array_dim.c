struct Pad {
    int tag;
    char data[sizeof(void *) <= 8 ? 16 : 32];
};
int main(void) {
    if (sizeof(((struct Pad *)0)->data) != 16) return 1;
    return 42;
}
