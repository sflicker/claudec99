//
// Created by scott on 6/29/25.
//

int sum3(int *a) {

    return a[0] + a[1] + a[2];
}

int main() {
    int nums[3] = {1, 2, 3};
    int sum = sum3(nums);
    return sum;       // 6
}
