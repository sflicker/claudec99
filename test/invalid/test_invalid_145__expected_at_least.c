/* Calling a variadic macro with fewer args than its fixed parameters is an error. */
int printf(const char *, ...);
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)

int main(void) {
    LOG();
    return 0;
}
