/* Stage 80: postfix increment through a pointer dereference lvalue.
 * x starts at 41, (*p)++ makes it 42. */
int main(void) {
    int x;
    int *p;

    x = 41;
    p = &x;
    (*p)++;

    return x;
}
