struct Pair { double a; double b; };
static struct Pair pairs[2];
int main(void) {
    pairs[0].a = 100.0;
    pairs[0].b = 58.0;
    pairs[1].a = pairs[0].a - pairs[0].b;
    return (int)pairs[1].a;   /* 42 */
}
