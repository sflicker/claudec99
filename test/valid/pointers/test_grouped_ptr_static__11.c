static int x = 11;
static int (*p);

int main() {
    p = &x;
    return *p;
}
