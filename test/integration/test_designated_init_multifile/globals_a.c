/* globals_a.c: defines a global struct with designated init */
struct Rect { int x; int y; int w; int h; };
struct Rect screen = { .w = 800, .h = 600, .x = 0, .y = 0 };

int screen_area(void) {
    return screen.w * screen.h;
}
