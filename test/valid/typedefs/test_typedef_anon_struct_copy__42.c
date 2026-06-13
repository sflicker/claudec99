typedef struct {
    int x;
    int y;
} P;

int main() {
    P a;
    P b;
    a.x = 10;
    a.y = 32;
    b = a;
    return b.x + b.y;
}
