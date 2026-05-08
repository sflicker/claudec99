/* Unnamed parameter in a function definition is not allowed. */
int f(int) {
    return 0;
}

int main() {
    return f(0);
}
