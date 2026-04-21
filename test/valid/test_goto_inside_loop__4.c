int main() {
    int a = 0;
    while (1) {
        a = a + 1;
        if (a == 4) goto done;
    }
done:
    return a;
}