struct Scale { double val; double factor; };
struct Scale s;

int main(void) {
    s.val    = 2.0;
    s.factor = 3.0;
    s.val *= s.factor;    /* 2.0 * 3.0 = 6.0 */
    return (int)s.val;
}
