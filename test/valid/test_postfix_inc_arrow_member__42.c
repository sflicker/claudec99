/* Stage 80: postfix increment on an arrow-access member.
 * cap starts at 41, ++ makes it 42. */
struct Buffer {
    int cap;
};

int main(void) {
    struct Buffer b;
    struct Buffer *p;

    p = &b;
    p->cap = 41;
    p->cap++;

    return p->cap;
}
