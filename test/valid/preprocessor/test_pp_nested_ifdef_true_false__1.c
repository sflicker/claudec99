#define OUTER

#ifdef OUTER
#ifdef INNER
int main() {
    return 42;
}
#else
int main() {
    return 1;
}
#endif
#else
int main() {
    return 2;
}
#endif
