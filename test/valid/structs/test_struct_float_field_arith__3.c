struct Vec2f { float x; float y; };
int main(void) {
    struct Vec2f v;
    v.x = 1.5f;
    v.y = 1.5f;
    float z = v.x + v.y;
    return (int)z;   /* 3 */
}
