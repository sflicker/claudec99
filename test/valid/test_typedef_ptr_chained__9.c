typedef int I;
typedef I *IPtr;

int main() {
    int x = 9;
    IPtr p = &x;
    return *p;
}
