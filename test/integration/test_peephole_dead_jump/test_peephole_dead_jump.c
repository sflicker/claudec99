int pass_through(int x) {
    goto done;
done:
    return x;
}

int main(void) {
    if (pass_through(42) != 42) return 1;
    if (pass_through(0) != 0) return 1;
    return 42;
}
