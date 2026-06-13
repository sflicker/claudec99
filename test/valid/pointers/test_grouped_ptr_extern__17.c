extern int (*p);
int x = 17;
int (*p);

int main() {
    p = &x;
    return *p;
}
