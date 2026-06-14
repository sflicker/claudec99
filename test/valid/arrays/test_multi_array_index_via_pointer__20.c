//
// Created by scott on 6/29/25.
//
int main() {
    int a[3][2] = {{2, 20}, 
                   {4, 5}, 
                   {6, 8}};
    int *p = &a[0][0];
    return *(p + 1);    // 20
}
