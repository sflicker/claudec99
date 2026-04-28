//
// Created by scott on 4/28/26.
//

int main() {
    int a[2];
    int *p = &a[1];
    *p =42;
    return a[1];   //expect 42cd
}