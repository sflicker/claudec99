#define SZ 5
struct Buf { int data[SZ + 5]; };
int main(void) {
    struct Buf b;
    b.data[0] = 42;
    return b.data[0];
}
