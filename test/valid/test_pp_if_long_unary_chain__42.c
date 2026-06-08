/* Stage 95-12: a long run of leading unary operators in a #if expression
 * must fold with no cap and no buffer overflow.  Each '-~x' folds
 * right-to-left to x+1, so 50 '-~' pairs applied to 0 yield exactly 50.
 * This both proves the old fixed ops[32] overflow is gone (100 operators)
 * and pins the right-to-left folding semantics (left-to-right would differ). */
#if ( -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~0 ) == 50
int main(void) { return 42; }
#else
int main(void) { return 1; }
#endif
