int main() {
    int x = 0;
    switch (1) {
        x = 99;
        case 1:
            x = 42;
            break;
        default:
            x = 7;
    }
    return x;
}
