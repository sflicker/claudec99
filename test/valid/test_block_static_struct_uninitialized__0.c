struct Counter { int n; int total; };

void tick(int v) {
    static struct Counter c;
    c.n++;
    c.total += v;
}

int main() {
    tick(5); tick(3);
    return 0;
}
