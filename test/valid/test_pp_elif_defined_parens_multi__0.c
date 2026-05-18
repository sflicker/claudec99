#define RED 0
#define GREEN 0
#define BLUE 1
#if defined(RED)
int main() { return 0; }
#elif defined(GREEN)
int main() { return 1; }
#elif defined(BLUE)
int main() { return 42; }
#endif
