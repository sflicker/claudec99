/* Calling a variadic function with fewer args than its fixed parameter count. */
int sum(int a, int b, ...) {
    return a + b;
}

int main(void) {
    return sum(1);
}
