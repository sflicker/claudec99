/* Variadic definition with unnamed fixed param and 7+ total args exercises
 * the unnamed-param parser relaxation and the 7+ arg stack-passing path. */
int log_val(const char *, ...) {
    return 1;
}

int main(void) {
    return log_val("hello", 1, 2, 3, 4, 5, 6, 7, 8, 9);
}
