#define A 1
#define B 0
#define C 1

#if A && C
int main() { return 42; }
#else
int main() { return 1; }
#endif
