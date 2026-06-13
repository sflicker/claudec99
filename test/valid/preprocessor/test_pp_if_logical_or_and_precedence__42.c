#define A 0
#define B 1
#define C 0

#if A || B && C
int main() { return 1; }
#else
int main() { return 42; }
#endif
