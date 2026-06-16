/* Stage 132: pointer == non-zero integer constant is accepted (extension).
 * p points to a stack variable; p == 5 is false so main returns 0. */
int main(void) {
    int x = 1;
    int *p = &x;
    return p == 5;
}
