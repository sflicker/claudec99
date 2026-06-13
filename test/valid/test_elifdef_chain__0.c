/* Only C is defined; the third branch (#elifdef C) should fire. */
#define C 1
#ifdef A
#define RESULT 10
#elifdef B
#define RESULT 20
#elifdef C
#define RESULT 30
#elifndef D
#define RESULT 40
#else
#define RESULT 50
#endif
int main(void) { return RESULT - 30; }
