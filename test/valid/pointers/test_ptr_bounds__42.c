static int data[6] = {10, 20, 30, 40, 50, 60};

int main(void) {
    int *lo = data;
    int *hi = data + 5;
    int *mid = data + 3;
    /* mid > lo and mid < hi */
    if (mid > lo && mid < hi)
        return *mid + 2;   /* 40 + 2 = 42 */
    return 0;
}
