#define A 1
#define B 0

#if A && B
int main() { return 1; }
#else
int main() { return 42; }
#endif
