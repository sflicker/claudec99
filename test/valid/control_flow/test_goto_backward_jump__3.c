int main() {
    int a = 0;
start:
    a = a + 1;
    if (a == 3) goto done;
    goto start;
done:
    return a;
}