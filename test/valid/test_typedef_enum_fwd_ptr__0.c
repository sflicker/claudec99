typedef enum Color Color;
enum Color { RED = 1, GREEN = 2, BLUE = 3 };
int color_to_int(Color c) { return (int)c; }
int main() { return color_to_int(GREEN) - 2; }
