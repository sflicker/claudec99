int set(void * a) {
    int * b = (int*)a;
    return *b;
}

int main(void) {
    int c = 42;
    int d = set(&c);
    return d;
}

