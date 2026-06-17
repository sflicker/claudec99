/* Anonymous padding bit-fields. */
struct WithPad {
    unsigned int x : 4;
    unsigned int   : 4; /* anonymous padding */
    unsigned int y : 8;
};

int main(void) {
    struct WithPad s;
    s.x = 3;
    s.y = 4;
    return (int)(s.x + s.y); /* 3 + 4 = 7 */
}
