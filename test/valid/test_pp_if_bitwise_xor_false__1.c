#define A 1
#define B 1

#if A ^ B
int main() { return 42; }
#else
int main() { return 1; }
#endif
