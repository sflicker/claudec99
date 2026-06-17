/* Zero-width bit-field forces new allocation unit. */
struct Split {
    unsigned int a : 8;
    unsigned int   : 0; /* force new storage unit */
    unsigned int b : 8;
};

int main(void) {
    struct Split s;
    s.a = 4;
    s.b = 5;
    return (int)(s.a + s.b); /* 4 + 5 = 9 */
}
