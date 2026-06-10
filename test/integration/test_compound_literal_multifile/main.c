struct Rect { int w; int h; };
int rect_area(struct Rect r);

int sum_arr(int *a, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += a[i];
    return s;
}

int main() {
    int area = rect_area((struct Rect){ .w = 6, .h = 7 });
    int total = sum_arr((int[]){ 1, 2, 3, 4, 5 }, 5);
    return area - total - 27;
}
