typedef int *IntPtr;

int main() {
    int x = 7;
    IntPtr p = &x;
    return *p;
}
