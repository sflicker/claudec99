int f(void) {
    static int cube[2][3][4] = {{{1}}};
    return cube[0][0][0];
}
int main() { return f(); }
