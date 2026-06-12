int main(void) {
    static int all = ~0;
    return (all & 0xFF) == 0xFF ? 0 : 1;   /* expect 0 */
}
