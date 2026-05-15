typedef int Integer;
typedef int *IntPtr;

int main() {
    Integer x = 5;
    IntPtr p = &x;
    return *p;
}
