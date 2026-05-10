struct Point {
    int x;
    int y;
};

struct Other {
    int x;
    int y;
};

int main() {
    struct Point p = {1, 2};
    struct Other q;

    q = p;
}
