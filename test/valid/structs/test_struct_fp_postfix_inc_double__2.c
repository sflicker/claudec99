struct Counter { double val; };

int main(void) {
    struct Counter c;
    c.val = 2.0;
    int old = (int)(c.val++);   /* returns 2 (old value); c.val becomes 3.0 */
    return old;
}
