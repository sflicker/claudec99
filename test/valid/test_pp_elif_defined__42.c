#define FIRST

#if defined(SECOND)
int main() { return 1; }
#elif defined(FIRST)
int main() { return 42; }
#else
int main() { return 2; }
#endif
