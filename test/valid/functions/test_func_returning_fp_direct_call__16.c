int add5(int value) {
    return value + 5;
}

int (*get_adder())(int) {
    return add5;
}

int main(void) {
    return get_adder()(11);
}
