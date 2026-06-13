int bump_and_get(int i) {
    static int tbl[3] = {[1] = 10, [0] = 5};
    tbl[i]++;
    return tbl[i];
}
int main() {
    bump_and_get(0); /* tbl[0] = 6 */
    bump_and_get(1); /* tbl[1] = 11 */
    return (bump_and_get(0) == 7 && bump_and_get(1) == 12) ? 0 : 1;
}
