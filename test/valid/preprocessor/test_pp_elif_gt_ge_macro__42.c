#define VERSION 6

#if VERSION > 8
int main() { return 1; }
#elif VERSION >= 6
int main() { return 42; }
#else
int main() { return 2; }
#endif
