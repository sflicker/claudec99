#define FLAG 1

int main(void) {
#if FLAG ? 1 : 0
    return 0;
#else
    return 1;
#endif
}
