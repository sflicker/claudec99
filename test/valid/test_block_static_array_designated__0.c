int get_slot(int i) {
    static int arr[4] = {[2] = 99, [0] = 1};
    return arr[i];
}
int main() {
    /* arr == {1, 0, 99, 0} */
    return (get_slot(0) == 1 && get_slot(1) == 0 &&
            get_slot(2) == 99 && get_slot(3) == 0) ? 0 : 1;
}
