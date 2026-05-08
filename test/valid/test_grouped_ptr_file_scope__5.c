int x = 5;
int (*p);

int main() {
    p = &x;
    return *p;
}
