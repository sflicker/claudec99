#define RED 0
#define GREEN 0
#define BLUE 1
#if RED
int main() { return 0; }
#elif GREEN
int main() { return 1; }
#elif BLUE
int main() { return 42; }
#endif
