#define A 10
#define B 4
#define C 2

#if C + A * B == 42
int main() { return 42; }
#else
int main() { return 1; }
#endif
