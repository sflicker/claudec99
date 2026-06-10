struct Point { int x; int y; };
int main() {
    int y = (struct Point){ .x = 1, .y = 2 }.y;
    return y;
}
