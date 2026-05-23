#if __SIZEOF_CHAR__ == 1 && __SIZEOF_SHORT__ == 2 && __SIZEOF_INT__ == 4 && __SIZEOF_SIZE_T__ == 8
int main() { return 42; }
#else
int main() { return 1; }
#endif
