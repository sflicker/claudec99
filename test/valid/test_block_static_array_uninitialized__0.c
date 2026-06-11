int bump_and_get(int i) {
    static int arr[4];
    arr[i]++;
    return arr[i];
}

int main() {
    bump_and_get(0); bump_and_get(0); bump_and_get(1);
    return (bump_and_get(0) == 3 && bump_and_get(1) == 2) ? 0 : 1;
}
