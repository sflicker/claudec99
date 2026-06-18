typedef int (*Adder)(int);

int add5(int value) {
    return value + 5;
}

Adder get_adder(void) {
    return add5;
}

int main(void) {
    Adder fn = get_adder();
    return fn(7);
}
