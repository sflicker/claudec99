/* Stage 80: prefix decrement on an arrow-access member returns the new
 * value. n starts at 43, --n makes it 42. */
struct Counter {
    int n;
};

int main(void) {
    struct Counter c;
    struct Counter *p;

    p = &c;
    p->n = 43;

    return --p->n;
}
