/* Stage 159: __asm__ / asm statements are parsed and discarded. */
int main(void) {
    int x = 5;
    __asm__ volatile ("" : : : "memory");
    x = x + 37;
    asm ("" : : :);
    return x;
}
