struct Item { int a; int b; int c; int d; };  /* 16 bytes each */
struct Item items[100];
int main(void) {
    items[99].d = 42;
    return items[99].d;
}
