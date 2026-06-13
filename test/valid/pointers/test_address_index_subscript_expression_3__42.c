//
// Created by scott on 4/28/26.
//
int main() {
    long a[2];
    long *p = &a[1];
    *p = 42;
    return a[1];
}