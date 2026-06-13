enum { X = 1 ? 42 : 0, Y = 0 ? 0 : 7 };
int main(void) { return X - 42 + Y - 7; }
