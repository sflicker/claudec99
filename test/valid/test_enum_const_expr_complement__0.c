enum Mask { ALL = ~0, NONE = 0 };
int main() { return (ALL & 0xFF) == 0xFF ? 0 : 1; }
