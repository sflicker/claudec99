/* Variadic function definition: return the fixed parameter. */
int fixed(int f, ...) {
    return f;
}

int main(void) {
    return fixed(42, 1, 2, 3);
}
