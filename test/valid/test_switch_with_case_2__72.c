int main() {
    int a = 2;
    int b = 0;

    switch (a) {
        case 1:
            b = 42;
            break;
        case 2:
            b = 72;
            break;
        default:
            b = 99;
            break;
    }

    return b;
}

