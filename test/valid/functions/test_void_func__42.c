void set(int * a) {
    *a = 42;
}

int main(void) {
    int a;
    set(&a);
    return a;
}

