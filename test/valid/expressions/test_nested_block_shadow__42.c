int main(void) {
    int x = 40;
    {
        int x = 1;
        x = x + 1;
    }
    return x + 2;
}
