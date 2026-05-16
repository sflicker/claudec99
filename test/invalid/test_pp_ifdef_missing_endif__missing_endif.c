#define ENABLED

#ifdef ENABLED
int main() {
    return 42;
}
/* MISSING #endif   should error */
