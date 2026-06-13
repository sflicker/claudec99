/* ALPHA is not defined; BETA is not defined - #elifndef BETA is taken. */
#ifdef ALPHA
#define RESULT 10
#elifndef BETA
#define RESULT 20
#else
#define RESULT 30
#endif
int main(void) { return RESULT - 20; }
