struct Pt { float x; float y; };

int main(void) {
    struct Pt pt;
    pt.x = 3.0f;
    ++pt.x;
    return (int)pt.x;   /* 4 */
}
