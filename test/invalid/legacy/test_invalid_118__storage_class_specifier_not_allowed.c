int main() {
    extern int x;   // INVALID: extern at block scope
    return x;
}
