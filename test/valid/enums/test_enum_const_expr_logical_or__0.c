enum { A = 1 || 0, B = 0 || 1, C = 0 || 0 };
int main(void) { return A - 1 + B - 1 + C; }
