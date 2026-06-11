struct P { int x; int y; };
struct P g = {1, 2};

int f(void) {
    static struct P p = g;
    return p.x;
}

int main() { return f(); }
