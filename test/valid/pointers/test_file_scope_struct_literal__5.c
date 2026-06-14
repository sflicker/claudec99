struct S { int x; int y; };
struct S *q = &(struct S){3, 5};
int main(void) { return q->y; }
