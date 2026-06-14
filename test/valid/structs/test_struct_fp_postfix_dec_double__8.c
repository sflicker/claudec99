struct Counter { double val; };

int main(void) {
    struct Counter c;
    c.val = 8.0;
    int old = (int)(c.val--);   /* returns 8; c.val becomes 7.0 */
    return old;
}
