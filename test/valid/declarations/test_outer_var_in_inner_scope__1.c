int main() {
    int a = 1;
    int result = 0;
    {
        int b = a;
        result = b;
    }
    return result;
}
