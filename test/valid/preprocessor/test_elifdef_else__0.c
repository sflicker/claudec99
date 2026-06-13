/* Neither ALPHA nor BETA is defined; #else should be taken. */
#ifdef ALPHA
#define RESULT 10
#elifdef BETA
#define RESULT 20
#else
#define RESULT 30
#endif
int main(void) { return RESULT - 30; }
