/* ALPHA not defined; BETA is defined - #elifndef BETA is false, #else taken. */
#define BETA 1
#ifdef ALPHA
#define RESULT 10
#elifndef BETA
#define RESULT 20
#else
#define RESULT 30
#endif
int main(void) { return RESULT - 30; }
