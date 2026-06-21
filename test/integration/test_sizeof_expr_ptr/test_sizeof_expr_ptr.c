struct Box { int w; int h; char label[20]; };
int main(void) {
    int a = sizeof(((struct Box *)0)->label);
    int b = sizeof(((struct Box *)0)->w);
    if (a != 20) return 1;
    if (b != 4)  return 2;
    return 42;
}
