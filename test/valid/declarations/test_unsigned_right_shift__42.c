/* Stage 40: unsigned right shift uses shr (zero-fill), not sar (sign-fill). */
int main() {
    unsigned int u = 128;
    unsigned int r = u >> 1;
    if (r == 64)
        return 42;
    return 1;
}
