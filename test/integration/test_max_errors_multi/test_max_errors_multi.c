/* Two separate conflicting function declarations.
 * With --max-errors=0 both errors should be reported. */

/* First error: return type mismatch for 'greet' */
int greet(int x);
void greet(int x);

/* Second error: parameter count mismatch for 'compute' */
int compute(int x);
int compute(int x, int y);

int main(void) {
    return 0;
}
