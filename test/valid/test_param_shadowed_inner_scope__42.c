int main() {
    return 42;
}

int shadow(int a) {
    {
        int a;
        a = 7;
        return a;
    }
}
