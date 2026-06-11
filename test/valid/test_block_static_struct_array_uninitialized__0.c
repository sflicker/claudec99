struct Pair { int a; int b; };
void set(int i, int a, int b) {
    static struct Pair pts[3];
    pts[i].a = a;
    pts[i].b = b;
}
int get_a(int i) {
    static struct Pair pts[3];
    return pts[i].a;
}
int main() { return 0; }
