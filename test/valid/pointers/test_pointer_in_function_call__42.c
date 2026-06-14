//
// Created by scott on 7/15/25.
//

int func(int *b) {
    *b = 42;
    return 0;        // currently not supporting void functions so just return 0 and don't use it in caller
}


int main() {
    int a = 10;
    int dummy = func(&a);      // need to handle the lack of assignment to the return.
    return a;
}
