#define VAR(n) var ## n

int main(void) {
    int var1;
    var1 = 42;
    return VAR(1);
}
