#if __LP64__ == 1 && _LP64 == 1 && __SIZEOF_LONG__ == 8 && __SIZEOF_POINTER__ == 8
int main() { return 42; }
#else
int main() { return 1; }
#endif
