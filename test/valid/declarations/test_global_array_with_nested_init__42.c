//
// Created by scott on 7/16/25.
//

int A[2][2] = {{36, 1},
               {2, 3}};

int main() {
    int sum = 0;
    sum += A[0][0];
    sum += A[0][1];
    sum += A[1][0];
    sum += A[1][1];
    return sum;
}
