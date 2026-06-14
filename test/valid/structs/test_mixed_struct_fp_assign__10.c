struct Vec2 { double x; double y; };
struct Vec2 global_v;

int main(void) {
    struct Vec2 local_v;
    global_v.x = 7.0;
    local_v.y  = 3.0;
    local_v.y += global_v.x;   /* 3.0 + 7.0 = 10.0 */
    return (int)local_v.y;
}
