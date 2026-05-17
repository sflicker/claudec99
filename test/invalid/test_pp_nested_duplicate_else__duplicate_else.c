#define OUTER

#ifdef OUTER
#ifdef INNER
int main() {
    return 1;
}
#else
int main() {
    return 2;
}
#else
int main() {
    return 3;
}
#endif
#endif
