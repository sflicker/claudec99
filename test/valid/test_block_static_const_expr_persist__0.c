int count(void) {
    static int step = 2 * 3;   /* 6 */
    static int n = 0;
    n += step;
    return n;
}
int main(void) {
    count(); count();
    return count() - 18;   /* 6+6+6 = 18; expect 0 */
}
