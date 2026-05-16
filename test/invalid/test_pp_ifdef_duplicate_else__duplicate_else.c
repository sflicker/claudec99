#define ENABLED

#ifdef ENABLED
int main() {
    return 42;
}
#else
int main() {
    return 1;
}
#else
int main() {
    return 2;
}
#endif
