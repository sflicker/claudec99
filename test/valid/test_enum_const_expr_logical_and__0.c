enum { A = 1 && 1, B = 1 && 0, C = 0 && 1 };
int main(void) { return A - 1 + B + C; }
