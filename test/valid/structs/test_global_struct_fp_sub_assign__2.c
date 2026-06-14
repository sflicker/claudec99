struct Pair { double a; double b; };
struct Pair p;

int main(void) {
    p.a = 5.0;
    p.b = 3.0;
    p.a -= p.b;           /* 5.0 - 3.0 = 2.0 */
    return (int)p.a;
}
