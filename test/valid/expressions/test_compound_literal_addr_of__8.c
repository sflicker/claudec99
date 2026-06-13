int main() {
    int *p = &(int){ 7 };
    *p = 8;
    return *p;
}
