#define VALUE 1

#if (-VALUE)
int main() { return 1; }
#elif (VALUE)
int main() { return 42; }
#else
int main() { return 2; }
#endif
