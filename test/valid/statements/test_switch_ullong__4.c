int main(void) {
    unsigned long long u = 4ULL;
    switch (u) {
    case 1ULL: return 1;
    case 4ULL: return 4;
    case 8ULL: return 8;
    default:   return 0;
    }
}
