int main(void) {
    int s = 0;
    for (int i = 0; i < 6; i++) {
        switch (i % 2) {
        case 0:  continue;
        default: break;
        }
        s += i;
    }
    return s;   /* 1 + 3 + 5 */
}
