#define ENABLED

#ifdef ENABLED
int main() {
    return 42;
}
#else
THIS IS INVALID SOURCE THAT SHOULD BE SKIPPED
#endif
