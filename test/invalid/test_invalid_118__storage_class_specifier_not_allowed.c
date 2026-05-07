int main() {
    static int x = 1;   // INVALID: storage class at block scope
    return x;
}
