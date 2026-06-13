int printf(const char *, ...);
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)

int main(void) {
    LOG("%s\n", "hello");
    return 0;   //expect 0
}
