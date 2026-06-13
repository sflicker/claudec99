int main(void) {
    static int page = 1 << 12;
    return page - 4096;   /* expect 0 */
}
