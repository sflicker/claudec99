#define X 1
#undef X

#ifndef X
int main() {
    return 42;
}
#else
int main() {
    return 1;
}
#endif
