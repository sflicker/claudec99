/* The SECOND branch should be taken: BETA is defined, ALPHA is not. */
#define BETA 1
#ifdef ALPHA
#define RESULT 10
#elifdef BETA
#define RESULT 20
#else
#define RESULT 30
#endif
int main(void) { return RESULT - 20; }
