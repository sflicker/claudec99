int main(void) {
    static int mask = 0xF0 | 0x0F;
    return mask - 255;   /* expect 0 */
}
