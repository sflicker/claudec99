struct Three { int a; int b; int c; };
int sum3(struct Three t) { return t.a + t.b + t.c; }
int main() {
    return sum3((struct Three){ .b = 5 });
}
