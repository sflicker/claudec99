int main(void) {
    static long sz = sizeof(long) * 8;
    return sz - 64;   /* expect 0 on LP64 */
}
