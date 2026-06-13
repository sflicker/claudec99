/* Stage 80: ++ requires a modifiable lvalue; an integer literal is an
 * rvalue and must be rejected. */
int main(void) {
    return 42++;
}
