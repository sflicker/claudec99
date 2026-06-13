/* Variadic function definition: return sum of two fixed params, extra args ignored. */
int sum(int a, int b, ...) {
    return a + b;
}

int main(void) {
    return sum(1, 2, 3, 4, 5);
}
