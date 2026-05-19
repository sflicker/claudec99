#define B 1

#if defined(A) || defined(B)
int main() { return 42; }
#else
int main() { return 1; }
#endif
