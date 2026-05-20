#define A 1

#if A << 1
int main() { return 42; }
#else
int main() { return 1; }
#endif
