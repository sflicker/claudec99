#define ENABLED
#ifndef ENABLED
int main() {
    return 1;
}
#elif 0
int main() {
    return 42;
}
#elif 1
int main() {
    return 2;
}
#endif
