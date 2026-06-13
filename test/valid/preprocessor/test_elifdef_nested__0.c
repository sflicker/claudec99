/* Outer block is inactive (OUTER not defined); inner #elifdef must not
   disturb the nesting depth. */
#ifdef OUTER
#  ifdef INNER_A
#  define RESULT 10
#  elifdef INNER_B
#  define RESULT 20
#  endif
#else
#define RESULT 99
#endif
int main(void) { return RESULT - 99; }
