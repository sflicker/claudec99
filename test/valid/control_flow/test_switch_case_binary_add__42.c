#define BASE 0

int main() {
    int x = 2;

    switch (x) {
        case BASE:
            return 0;
        case BASE + 1:
            return 1;
        case BASE + 2:
            return 42;
        default:
            return 3;
    }
}
