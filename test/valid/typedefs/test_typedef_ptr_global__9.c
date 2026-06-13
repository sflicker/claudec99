typedef int *P;
P a, b;

int main() {
    int x = 9;
    a = &x;
    b = a;
    return *b;
}
