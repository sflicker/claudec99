int add5(int value) {
    return value + 5;
}

int (*get_adder())(int) {
    return add5;
}

int main(void) {
    int (*fn)(int);
    fn = get_adder();
    return fn(7);
}
