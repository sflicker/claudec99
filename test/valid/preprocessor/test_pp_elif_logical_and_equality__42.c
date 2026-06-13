#define PRIORITY 1
#define SEVERITY 2

#if PRIORITY == 1 && SEVERITY == 1
int main() { return 1; }
#elif PRIORITY == 1 && SEVERITY == 2
int main() { return 42; }
#else
int main() { return 2; }
#endif
