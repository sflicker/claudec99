#define ENABLED

#ifdef ENABLED
#define VALUE 42
#else
#define VALUE 1
#endif

int main() {
    return VALUE;
}
