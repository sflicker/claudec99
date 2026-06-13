#define VERSION 10

#if VERSION <= 10
int main() { return 42; }
#else
int main() { return 1; }
#endif
