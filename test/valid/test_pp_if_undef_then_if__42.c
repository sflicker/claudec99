#define FLAG 1
#undef FLAG

#if FLAG
int main() { return 1; }
#else
int main() { return 42; }
#endif
