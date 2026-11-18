int main(void) {
    int x = 100000;
    int r = 0;
    switch (x) {
    case -5:     r = 1; break;
    case 4096:   r = 2; break;
    case 100000: r = 3; break;
    default:     r = 9;
    }
    return r;
}
