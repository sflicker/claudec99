#define ENABLED
#ifdef ENABLED
int main() {
    return 1;
}
#elif 1
int main() {
    return 42;
}
#endif
