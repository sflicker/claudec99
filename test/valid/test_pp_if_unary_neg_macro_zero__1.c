#define FLAG 0

#if -FLAG
int main() { return 42; }
#else
int main() { return 1; }
#endif
