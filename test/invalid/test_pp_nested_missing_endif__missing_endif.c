#define OUTER

#ifdef OUTER
#ifdef INNER
int main() {
    return 1;
}
#endif
/* INVALID: outer #endif is missing */
