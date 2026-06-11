int n = 5;

int f(void) {
    static int arr[3] = n;
    return arr[0];
}

int main() { return f(); }
