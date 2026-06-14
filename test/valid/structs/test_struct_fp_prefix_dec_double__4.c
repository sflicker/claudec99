struct Vec { double x; double y; };

int main(void) {
    struct Vec v;
    v.x = 5.0;
    --v.x;
    return (int)v.x;   /* 4 */
}
